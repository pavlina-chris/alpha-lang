/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "lex.h"
#include "../stringlist.h"
#include "../filesystem.h"
#include "../error.h"
#include "../collector.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void lexer_init (char const *file, struct env *env, struct lex *lex)
{
  lex->file = file;
  lex->env = env;
  lex->tokens = NULL;
  lex->n_tokens = 0;
  lex->token_idx = 0;
  lex->text = NULL;
  lex->text_len = 0;
  lex->lines = NULL;
}

void lexer_free (struct lex *lex)
{
  size_t i;
  if (lex->tokens) {
    for (i = 0; i < lex->n_tokens; ++i) {
      if (lex->tokens[i].value) free (lex->tokens[i].value);
    }
    free (lex->tokens);
  }
  if (lex->text)
    free (lex->text);
  if (lex->lines)
    free (lex->lines);
}

/* Get the entire contents of the file into a character array. The array will
 * be put at *text_ptr (and should be released with free()); the size will be
 * put at *text_sz_ptr. This function guarantees a NUL character at the end of
 * the array, at (*text_ptr)[*text_sz_ptr]. */
#define CHUNK_SIZE 512
static void get_text (char **text_ptr, size_t *text_sz_ptr, const char *file)
{
  FILE *f = NULL;
  char buffer[CHUNK_SIZE];
  size_t text_buf_sz;

  f = fopen (file, "r");
  if (!f) goto err;

  // text_buf_sz should be equal to the size of the file plus one, unless we
  // were given something funny like a named pipe
  text_buf_sz = size_of_f (f) + 1;
  if (text_buf_sz == 1 && errno) {
    // Bizarre - stat() error after successful fopen()
    goto err;
  }

  *text_ptr = malloc (text_buf_sz);
  if (!*text_ptr) goto err;
  *text_sz_ptr = 0;

  while (1) {
    size_t n_read = fread (buffer, 1, CHUNK_SIZE, f);
    int errno_temp = errno;
    if (*text_sz_ptr + n_read > (text_buf_sz - 1)) {
      char *new_text = realloc (*text_ptr, 2 * (text_buf_sz - 1) + 1);
      if (!new_text) goto err;
      *text_ptr = new_text;
      text_buf_sz = (text_buf_sz - 1) * 2 + 1;
    }
    memcpy (*text_ptr + *text_sz_ptr, buffer, n_read);
    *text_sz_ptr += n_read;

    if (feof (f)) break;
    else if (ferror (f)) {
      errno = errno_temp;
      goto err;
    }
  }
  (*text_ptr)[*text_sz_ptr] = 0;

  fclose (f);
  f = NULL;
  return;

 err:
  {
    int errno_temp = errno;
    if (*text_ptr) free (*text_ptr);
    if (f) fclose (f);
    errno = errno_temp;
    error_errno ();
  }
}

/* Fill in the 'lines' array */
static void mark_lines (struct lex *lex)
{
  /* First, count the number of newlines */
  size_t pos, count = 1;
  for (pos = 0; pos < lex->text_len; ++pos) {
    if (lex->text[pos] == '\n') ++count;
  }
  
  /* Allocate the array */
  lex->lines = malloc (count * sizeof (*lex->lines));
  if (!lex->lines) error_errno ();

  /* Fill in the array */
  lex->lines[0] = lex->text;
  count = 1;
  for (pos = 0; pos < lex->text_len; ++pos) {
    if (lex->text[pos] == '\n') {
      /* Note that we can set the line at pos+1 - get_text() ensures a final
       * NUL. */
      lex->lines[count] = &lex->text[pos + 1];
      ++count;
    }
  }
}

static void lexer_read_text (struct lex *lex)
{
  // Get the text into the text array. This will exit if there is an error
  get_text (&lex->text, &lex->text_len, lex->file);
  mark_lines (lex);

  // text_len is an upper bound for n_tokens
  lex->tokens = malloc (lex->text_len * sizeof (*lex->tokens));
  if (!lex->tokens) {
    int errno_temp = errno;
    free (lex->text);
    errno = errno_temp;
    error_errno ();
  }
}

static void consume_string (struct lex *lex, struct collector *coll,
                            size_t *nline, size_t *ncol, size_t *pos)
{
  size_t first_col = *ncol;
  int in_escape = 0, found_end = 0;

  for (; *pos < lex->text_len; ++*pos, ++*ncol) {
    char ch = lex->text[*pos];
    if (ch < ' ' || ch > '~') {
      char char_as_string[2] = {ch, 0};
      struct token temp = {*nline, *ncol, 0, char_as_string};
      cerror_at (lex, &temp, "unexpected character '\\x%02x'", ch);
    }
    coll_append (coll, ch);
    if (ch == '"' && !in_escape && *ncol != first_col) {
      found_end = 1;
      break;
    }
    if (ch == '\\') in_escape = !in_escape;
  }

  if (!found_end) {
    struct token temp = {*nline, first_col, 0, "\""};
    cerror_at (lex, &temp, "unexpected end of line while parsing string");
  }

  struct token tok = {*nline, first_col, T_STRING, coll_get (coll)};
  coll_clear (coll);
  lex->tokens[lex->n_tokens] = tok;
  ++lex->n_tokens;
}

static void consume_oper (struct lex *lex, struct collector *coll,
                          size_t *nline, size_t *ncol, size_t *pos)
{
  char c1, c2, c3;
  char const *oper = NULL;
  c1 = lex->text[*pos];
  c2 = (c1 ? lex->text[*pos + 1] : 0);
  c3 = (c2 ? lex->text[*pos + 2] : 0);

  switch (c1) {
  case '/':
    switch (c2) {
    case '=': oper = "/="; break;
    default:  oper = "/";
    } break;
  case '+':
    switch (c2) {
    case '+': oper = "++"; break;
    case '=': oper = "+="; break;
    default:  oper = "+";
    } break;
  case '-':
    switch (c2) {
    case '-': oper = "--"; break;
    case '=': oper = "-="; break;
    default:  oper = "-";
    } break;
  case '~':
    oper = "~"; break;
  case '*':
    switch (c2) {
    case '=': oper = "*="; break;
    default:  oper = "*";
    } break;
  case '%':
    switch (c2) {
    case '%':
      switch (c3) {
      case '=': oper = "%%="; break;
      default:  oper = "%%";
      } break;
    case '=': oper = "%="; break;
    default:  oper = "%";
    } break;
  case '<':
    switch (c2) {
    case '<':
      switch (c3) {
      case '=': oper = "<<="; break;
      default:  oper = "<<"; break;
      } break;
    case '=': oper = "<="; break;
    default:  oper = "<";
    } break;
  case '>':
    switch (c2) {
    case '>':
      switch (c3) {
      case '=': oper = ">>="; break;
      default:  oper = ">>"; break;
      } break;
    case '=': oper = ">="; break;
    default:  oper = ">";
    } break;
  case '&':
    switch (c2) {
    case '&': oper = "&&"; break;
    case '=': oper = "&="; break;
    default:  oper = "&";
    } break;
  case '^':
    switch (c2) {
    case '=': oper = "^="; break;
    default:  oper = "^";
    } break;
  case '|':
    switch (c2) {
    case '|': oper = "||"; break;
    case '=': oper = "|="; break;
    default:  oper = "|";
    } break;
  case '!':
    switch (c2) {
    case '=':
      switch (c3) {
      case '=': oper = "!=="; break;
      default: oper = "!="; break;
      } break;
    default:  oper = "!"; break;
    } break;
  case '=':
    switch (c2) {
    case '=':
      switch (c3) {
      case '=': oper = "==="; break;
      default:  oper = "=="; break;
      } break;
    default:  oper = "=";
    } break;
  case '(':
    oper = "("; break;
  case ')':
    oper = ")"; break;
  case '[':
    oper = "["; break;
  case ']':
    oper = "]"; break;
  case '{':
    oper = "{"; break;
  case '}':
    oper = "}"; break;
  case ',':
    oper = ","; break;
  case ':':
    switch (c2) {
    case '=': oper = ":="; break;
    default:  oper = ":"; break;
    } break;
  case ';':
    oper = ";"; break;
  case '.':
    if (c2 == '.' && c3 == '.') {
      oper = "..."; break;
    } else {
      oper = "."; break;
    }
  case '?':
    oper = "?"; break;
  }

  if (!oper) {
    char char_as_string[2] = {c1, 0};
    struct token temp = {*nline, *ncol, 0, char_as_string};
    cerror_at (lex, &temp, "unexpected character '%c'", c1);
  }

  // Use the collector as a strdup - it has standard error handling
  coll_append_s (coll, oper);
  struct token tok = {*nline, *ncol, T_OPER, coll_get (coll)};
  *ncol += coll_len (coll);
  *pos += coll_len (coll);
  coll_clear (coll);
  lex->tokens[lex->n_tokens] = tok;
  ++lex->n_tokens;
}

static void consume_block_comment (struct lex *lex, struct collector *coll,
                                   size_t *nline, size_t *ncol, size_t *pos)
{
  /* This function is designed to be started just after the opening marker.
   * It runs to the closing marker, recursing to process nesting.
   * Complain if we hit the end of the file. */

  /* Mark the starting point, for any possible error message */
  size_t first_col = *ncol, first_line = *nline;

  while (*pos < lex->text_len) {
    if (lex->text[*pos] == '/' && lex->text[*pos + 1] == '*') {
      /* Nested comment */
      *ncol += 2;
      *pos += 2;
      consume_block_comment (lex, coll, nline, ncol, pos);

    } else if (lex->text[*pos] == '*' && lex->text[*pos + 1] == '/') {
      *ncol += 2;
      *pos += 2;
      return;

    } else if (lex->text[*pos] == '\n') {
      ++*nline;
      *ncol = 0;
      ++*pos;

    } else {
      ++*ncol;
      ++*pos;
    }
  }

  cerror_eof (lex, "comment started at %d:%d", first_line + 1,
              first_col + 1);
}

static void consume_oper_or_comment (struct lex *lex, struct collector *coll,
                                     size_t *nline, size_t *ncol, size_t *pos)
{
  if (lex->text[*pos] == '/' && lex->text[*pos + 1] == '/') {
    // Line comment - skip the rest of the line
    for (; *pos < lex->text_len; ++*pos) {
      if (lex->text[*pos] == '\n') {
        break;
      } else {
        ++*ncol;
      }
    }

  } else if (lex->text[*pos] == '/' && lex->text[*pos + 1] == '*') {
    // Block comment
    *pos += 2;
    *ncol += 2;
    consume_block_comment (lex, coll, nline, ncol, pos);

  } else
    consume_oper (lex, coll, nline, ncol, pos);
}

static void consume_extrastandard (struct lex *lex, struct collector *coll,
                                   size_t *nline, size_t *ncol, size_t *pos)
{
  int break_for = 0;
  size_t first_col = *ncol;

  for (; *pos < lex->text_len; ++*pos, ++*ncol) {
    char ch = lex->text[*pos];
    if (*ncol == first_col || *ncol == (first_col + 1)) {
      if (ch != '$') {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "extrastandard identifier must start "
                   "with $$");
      }
      coll_append (coll, ch);
      continue;
    }
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      // Not valid as the first character after $$
      if (*ncol == first_col + 2) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
      }
      coll_append (coll, ch);
      break;
    case '_': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z':
      coll_append (coll, ch);
      break;
    default:
      break_for = 1;
      break;
    }
    if (break_for) break;
  }
  struct token tok = {*nline, first_col, T_EXTRA, coll_get (coll)};
  coll_clear (coll);
  lex->tokens[lex->n_tokens] = tok;
  ++lex->n_tokens;
}

static void consume_word (struct lex *lex, struct collector *coll,
                          size_t *nline, size_t *ncol, size_t *pos)
{
  int break_for = 0;
  int has_at = 0;
  size_t first_col = *ncol;

  for (; *pos < lex->text_len; ++*pos, ++*ncol) {
    char ch = lex->text[*pos];
    switch (ch) {
    case '@':
      // Only valid as the first character
      if (*ncol != first_col) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
      }
      has_at = 1;
      coll_append (coll, ch);
      break;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      // Not valid as the first character. Don't worry about the
      // !has_at case, because if the first character were a digit,
      // this wouldn't have been picked up as a word anyway
      if (has_at) {
        if (*ncol == first_col + 1) {
          char char_as_string[2] = {ch, 0};
          struct token temp = {*nline, *ncol, 0, char_as_string};
          cerror_at (lex, &temp, "unexpected character '%c'", ch);
        }
      }
      coll_append (coll, ch);
      break;
    case '_': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z':
      coll_append (coll, ch);
      break;
    default:
      break_for = 1;
      break;
    }
    if (break_for) break;
  }
  struct token tok = {*nline, first_col, T_WORD, coll_get (coll)};
  coll_clear (coll);
  lex->tokens[lex->n_tokens] = tok;
  ++lex->n_tokens;
}

static void consume_number (struct lex *lex, struct collector *coll,
                            size_t *nline, size_t *ncol, size_t *pos)
{
  int break_for = 0;
  int radix = 10;
  int radix_allowed = 0;
  int dot_allowed = 1;
  int exp_allowed = 1;
  size_t first_col = *ncol;
  int type = T_INT;
  int found_exp = 0;
  size_t exp_col = 0;

  for (; *pos < lex->text_len; ++*pos, ++*ncol) {
    char ch = lex->text[*pos];
    switch (ch) {
    case '+': case '-':
      // Stop character or exponent sign.
      if (found_exp && *ncol == exp_col + 1) {
        coll_append (coll, ch);
      } else {
        break_for = 1;
      }
      break;
    case '0':
      if (*ncol == first_col) {
        // Starting a number with 0 allows a radix to be specified.
        radix_allowed = 1;
      }
      coll_append (coll, ch);
      break;
    case '1':
      coll_append (coll, ch);
      break;
    case '2': case '3': case '4': case '5': case '6': case '7':
      // These characters are allowed in octal and above
      if (radix < 8) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case '8': case '9':
      // These characters are allowed in decimal and above
      if (radix < 10) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case 'A': case 'C': case 'F': case 'a': case 'c': case 'f':
      // These characters are allowed in hexadecimal
      if (radix < 16) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case 'B': case 'b':
      // This is a hex 0xB or a binary radix specifier
      if (*ncol == first_col + 1 && radix_allowed) {
        exp_allowed = dot_allowed = 0;
        radix = 2;
      } else if (radix < 16) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case 'D': case 'd':
      // This is a hex 0xD or a decimal radix specifier
      if (*ncol == first_col + 1 && radix_allowed) {
        exp_allowed = dot_allowed = 0;
        radix = 10;
      } else if (radix < 16) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case 'O': case 'o':
      if (*ncol == first_col + 1 && radix_allowed) {
        exp_allowed = dot_allowed = 0;
        radix = 8;
      } else {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
      }
      coll_append (coll, ch);
      break;
    case 'X': case 'x':
      if (*ncol == first_col + 1 && radix_allowed) {
        exp_allowed = dot_allowed = 0;
        radix = 16;
      } else {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
      }
      coll_append (coll, ch);
      break;
    case 'E': case 'e':
      // This is a hex 0xE or an exponent specifier
      if (exp_allowed) {
        exp_allowed = dot_allowed = 0;
        type = T_REAL;
        found_exp = 1;
        exp_col = *ncol;
      } else if (radix < 16) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected '%c' in base %d",
                   ch, radix);
      }
      coll_append (coll, ch);
      break;
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'Y': case 'Z':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'y': case 'z':
      {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
        break;
      }
    case '.':
      if (!dot_allowed) {
        char char_as_string[2] = {ch, 0};
        struct token temp = {*nline, *ncol, 0, char_as_string};
        cerror_at (lex, &temp, "unexpected character '%c'", ch);
      }
      type = T_REAL;
      dot_allowed = 0;
      coll_append (coll, ch);
      break;
    default:
      break_for = 1;
      break;
    }
    if (break_for) break;
  }
  struct token tok = {*nline, first_col, type, coll_get (coll)};
  coll_clear (coll);
  lex->tokens[lex->n_tokens] = tok;
  ++lex->n_tokens;
}

void lexer_lex (struct lex *lex)
{
  lexer_read_text (lex);
  struct collector coll;
  size_t nline = 0, ncol = 0, pos = 0;

  coll_init (&coll);

  while (pos < lex->text_len) {
    switch (lex->text[pos]) {
    case ' ': case 0x09: case 0x0b: case 0x0c: case 0x0d:
      /* Whitespace */
      ++pos;
      ++ncol;
      continue;

    case 0x0a:
      /* Newline */
      ++nline;
      ++pos;
      ncol = 0;
      break;

    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      consume_number (lex, &coll, &nline, &ncol, &pos);
      break;

    case '@': case '_': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
      consume_word (lex, &coll, &nline, &ncol, &pos);
      break;

    case '$':
      consume_extrastandard (lex, &coll, &nline, &ncol, &pos);
      break;

    case '/':
      consume_oper_or_comment (lex, &coll, &nline, &ncol, &pos);
      break;

    case '+': case '-': case '~': case '*': case '%': case '<':
    case '>': case '&': case '^': case '|': case '!': case '=':
    case '(': case ')': case '[': case ']': case '{': case '}':
    case ',': case ';': case ':': case '.': case '?':
      consume_oper (lex, &coll, &nline, &ncol, &pos);
      break;

    case '"':
      consume_string (lex, &coll, &nline, &ncol, &pos);
      break;

    default:
      {
        char char_as_string[2] = {lex->text[pos], 0};
        struct token temp_token = {nline, ncol, 0, char_as_string};
        cerror_at (lex, &temp_token, "unexpected character '%c'",
                   lex->text[pos]);
      }
    }
  }

  coll_free (&coll);
}

struct token *lexer_next (struct lex *lex)
{
  if (lex->token_idx >= lex->n_tokens)
    return NULL;
  return &lex->tokens[lex->token_idx++];
}

struct token *lexer_peek (struct lex *lex)
{
  if (lex->token_idx >= lex->n_tokens)
    return NULL;
  return &lex->tokens[lex->token_idx];
}

struct token *lexer_last (struct lex *lex)
{
  if (lex->token_idx < 2) return NULL;
  return &lex->tokens[lex->token_idx - 2];
}
