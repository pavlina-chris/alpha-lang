/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _LEX_LEX_H
#define _LEX_LEX_H 1

#include "../read_args.h"
#include "../env.h"
#include "../stringlist.h"
#include "token.h"
#include <stdio.h>

/* Lexer structs and functions. */

struct lex {
  char const *file;
  struct env *env;

  struct token *tokens;
  size_t n_tokens;
  size_t token_idx;

  char *text;
  size_t text_len;

  char **lines;
};

/* Initialise the lexer.
 * file: File to lex
 * env: Compile environment
 * lex: A 'struct lex'
 */
void lexer_init (char const *file, struct env *env, struct lex *lex);

/* Free the lexer's used memory */
void lexer_free (struct lex *lex);

/* Run the lexer. May exit with errors. */
void lexer_lex (struct lex *lex);

/* Get the next token. */
struct token *lexer_next (struct lex *lex);

/* Peek at the next token. */
struct token *lexer_peek (struct lex *lex);

/* Get the previous token. */
struct token *lexer_last (struct lex *lex);

#endif /* _LEX_LEX_H */
