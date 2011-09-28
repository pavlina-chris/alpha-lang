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
#include <assert.h>

void
lexer_init (char const *file, struct env *env, struct lex *lex)
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

void
lexer_free (struct lex *lex)
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
static void
get_text (char **text_ptr, size_t *text_sz_ptr, const char *file)
{
    FILE *f = NULL;
    char buffer[CHUNK_SIZE];
    char *new_text;
    size_t text_buf_sz, n_read;
    int errno_temp;

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
        n_read = fread (buffer, 1, CHUNK_SIZE, f);
        errno_temp = errno;
        if (*text_sz_ptr + n_read > (text_buf_sz - 1)) {
            new_text = realloc (*text_ptr, 2 * (text_buf_sz - 1) + 1);
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
    errno_temp = errno;
    if (*text_ptr) free (*text_ptr);
    if (f) fclose (f);
    errno = errno_temp;
    error_errno ();
}

/* Fill in the 'lines' array */
static void
mark_lines (struct lex *lex)
{
    size_t pos, count = 1;

    /* First, count the number of newlines */
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
            /* Note that we can set the line at pos+1 - get_text() ensures a
             * final NUL. */
            lex->lines[count] = &lex->text[pos + 1];
            ++count;
        }
    }
}

static void
lexer_read_text (struct lex *lex)
{
    int errno_temp;

    // Get the text into the text array. This will exit if there is an error
    get_text (&lex->text, &lex->text_len, lex->file);
    mark_lines (lex);

    // text_len is an upper bound for n_tokens
    lex->tokens = malloc (lex->text_len * sizeof (*lex->tokens));
    if (!lex->tokens) {
        errno_temp = errno;
        free (lex->text);
        errno = errno_temp;
        error_errno ();
    }
}

static void
unexpected_char (struct lex *lex, char ch, size_t nline, size_t ncol,
                 const char *suffix)
{
    char char_as_string[2] = {ch, 0};
    struct token temp = {nline, ncol, 0, char_as_string};
    cerror_at (lex, &temp, "unexpected character '\\x%02x'%s", ch, suffix);
}

static void
consume_string (struct lex *lex, struct collector *coll,
                size_t *nline, size_t *ncol, size_t *pos)
{
    size_t first_col = *ncol;
    int in_escape = 0, found_end = 0;
    char ch;

    for (; *pos < lex->text_len; ++*pos, ++*ncol) {
        ch = lex->text[*pos];
        if (ch < ' ' || ch > '~') {
            unexpected_char (lex, ch, *nline, *ncol, "");
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

static void
consume_oper (struct lex *lex, struct collector *coll,
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
        unexpected_char (lex, c1, *nline, *ncol, "");
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
                unexpected_char (lex, ch, *nline, *ncol, "");
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
                unexpected_char (lex, ch, *nline, *ncol, "");
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
                    unexpected_char (lex, ch, *nline, *ncol, "");
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

static void
consume_number (struct lex *lex, struct collector *coll,
                size_t *nline, size_t *ncol, size_t *pos)
{
    /* This is the number consumer. It handles all numbers except for character
     * values (those are, after all, technically u8 and u32 values). Numbers
     * are of the following basic forms:
     *
     * 0.23   0.23e+2   0.23e-2   0.23f   0.23F   0.23e+2f     (etc)
     * 1234  1234:u  1234:u32  142:i8                          (etc)
     * 0x23a  0xc52:u32  0X2aF:i16                             (etc)
     * 0o12  0O34:i8                                           (etc)
     *
     * Floats starting with a point are not accepted (prefix a zero).
     */

    int radix = 10, charval, type = T_INT;
    size_t first_col = *ncol;
    enum {prepoint, postpoint, exponent, expofirstdig, expomoredigs,
          intonly, typespec, muststop} state = prepoint;

    /* First check for a radix prefix. */
    if (lex->text[*pos] == '0') {
        switch (lex->text[*pos + 1]) {
        case 'x': case 'X':
            radix = 16;
            break;
        case 'O': case 'o':
            radix = 8;
            break;
        }
        if (radix != 10) {
            coll_append (coll, '0');
            coll_append (coll, lex->text[*pos + 1]);
            *pos += 2;
            *ncol += 2;
            state = intonly;
        }
    }

/* Macro: take a character and return the value, or -1 if invalid.
 * Currently Alpha doesn't accept any radix above 16, but this macro does. */
#define CHARVAL(c)                                                      \
    ((c >= '0' && c <= '9') ? (c - '0')                                 \
     : (c >= 'a' && c <= 'z') ? (c - 'a' + 10)                          \
     : (c >= 'A' && c <= 'Z') ? (c - 'A' + 10)                          \
     : -1)
#define STOPCHAR(c)                                                     \
    ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F'))

    for (; *pos < lex->text_len; ++*pos, ++*ncol) {
        char ch = lex->text[*pos];
        switch (state) {
        case prepoint:
            /* Before a decimal point, we can accept the following things:
             * - digits: just collect them
             * - decimal point: set type to T_REAL, switch to prepoint
             * - colon: switch to typespec
             * - 'e': set type to T_REAL, switch to exponent
             * - 'f': set type to T_REAL, switch to muststop
             */
            switch (ch) {
            case '.':
                coll_append (coll, ch);
                type = T_REAL;
                state = postpoint;
                break;
            case ':':
                coll_append (coll, ch);
                state = typespec;
                break;
            case 'e': case 'E':
                coll_append (coll, ch);
                type = T_REAL;
                state = exponent;
                break;
            case 'f': case 'F':
                coll_append (coll, ch);
                type = T_REAL;
                state = muststop;
                break;
            default:
                if (STOPCHAR (ch)) goto out;
                charval = CHARVAL (ch);
                if (charval == -1 || charval >= 10)
                    unexpected_char (lex, ch, *nline, *ncol, "");
                coll_append (coll, ch);
            }
            break;
        case postpoint:
            /* After a decimal point, we can accept the following things:
             * - digits: just collect them
             * - 'e': switch to exponent
             * - 'f': switch to muststop
             */
            switch (ch) {
            case 'e': case 'E':
                coll_append (coll, ch);
                state = exponent;
                break;
            case 'f': case 'F':
                coll_append (coll, ch);
                state = muststop;
                break;
            default:
                if (STOPCHAR (ch)) goto out;
                charval = CHARVAL (ch);
                if (charval == -1 || charval >= 10)
                    unexpected_char (lex, ch, *nline, *ncol, "");
                coll_append (coll, ch);
            }
            break;
        case exponent:
            /* After an exponent starter, we can accept the following things:
             * '+', '-': switch to expofirstdig
             * digits: switch to expomoredigs
             */
            switch (ch) {
            case '+': case '-':
                coll_append (coll, ch);
                state = expofirstdig;
                break;
            default:
                charval = CHARVAL (ch);
                if (charval == -1 || charval >= 10)
                    unexpected_char (lex, ch, *nline, *ncol, "");
                coll_append (coll, ch);
                state = expomoredigs;
            }
            break;
        case expofirstdig:
            /* After e or E, at least one digit is expected. We can accept the
             * following things:
             * digits: switch to expomoredigs
             */
            charval = CHARVAL (ch);
            if (charval == -1 || charval >= 10)
                unexpected_char (lex, ch, *nline, *ncol, "");
            coll_append (coll, ch);
            state = expomoredigs;
            break;
        case expomoredigs:
            /* After the first digit past e/E, we can accept the following
             * things:
             * digits: just collect
             * 'f': switch to muststop
             * ':': switch to typespec
             */
            if (ch == 'f') {
                coll_append (coll, ch);
                state = muststop;
                break;
            } else if (ch == ':') {
                coll_append (coll, ch);
                state = typespec;
                break;
            }
            if (STOPCHAR (ch)) {
                goto out;
            }
            charval = CHARVAL (ch);
            if (charval == -1 || charval >= 10)
                unexpected_char (lex, ch, *nline, *ncol, "");
            coll_append (coll, ch);
            break;
        case intonly:
            /* This state is used when we know for a fact that we can only have
             * int stuff. Allow digits in the radix, as well as a switch to
             * typespec. */
            if (ch == ':') {
                coll_append (coll, ch);
                state = typespec;
                break;
            } else if (STOPCHAR (ch)) {
                goto out;
            }
            charval = CHARVAL (ch);
            if (charval == -1)
                unexpected_char (lex, ch, *nline, *ncol, "");
            else if (charval >= radix)
                unexpected_char (lex, ch, *nline, *ncol, " in this radix");
            else
                coll_append (coll, ch);
            break;

        case typespec:
            /* A number may have a typespec. Valid typespecs are
             * f{16,32,64}, float, double, {i,u}{8,16,32,64}, int, unsigned,
             * size, ssize. We don't validate them here; we'll take any
             * alphanum. */
            /* Lazy shortcut: CHARVAL() returns >=0 for alphanum */
            if (CHARVAL (ch) >= 0) {
                if (lex->text[*pos - 1] == ':' &&
                    (ch == 'f' || ch == 'd')) {
                    /* Detect typespecs :f.* and :d.* - the only valid names
                     * matching those are f16, f32, f64, float, double. Switch
                     * to T_REAL. */
                    type = T_REAL;
                }

                coll_append (coll, ch);
                break;
            } else
                goto out;
        case muststop:
            /* This state is used when the end of the number is required.
             * Complain if the next character is anything that could be part
             * of a number (this stops things like 3.2f9); finish otherwise.
             */
            if (!STOPCHAR (ch))
                unexpected_char (lex, ch, *nline, *ncol, "");
            goto out;
        default:
            assert (!"INVALID STATE IN LOOP");
        }
    }
out:
    ;

#undef CHARVAL

    struct token tok = {*nline, first_col, type, coll_get (coll)};
    coll_clear (coll);
    lex->tokens[lex->n_tokens] = tok;
    ++lex->n_tokens;
}

void
lexer_lex (struct lex *lex)
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
            unexpected_char (lex, lex->text[pos], nline, ncol, "");
        }
    }

    coll_free (&coll);
}

struct token *
lexer_next (struct lex *lex)
{
    if (lex->token_idx >= lex->n_tokens)
        return NULL;
    return &lex->tokens[lex->token_idx++];
}

struct token *
lexer_peek (struct lex *lex)
{
    if (lex->token_idx >= lex->n_tokens)
        return NULL;
    return &lex->tokens[lex->token_idx];
}

struct token *
lexer_last (struct lex *lex)
{
    if (lex->token_idx < 2) return NULL;
    return &lex->tokens[lex->token_idx - 2];
}
