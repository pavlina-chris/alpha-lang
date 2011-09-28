/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "lex.h"
#include <stdio.h>
#include <string.h>

static const char *TYPES[] = {"-------", "STRING", "WORD", "INT",
                              "REAL", "OPER", "EXTRA", "SPECIAL"};

void
print_token (FILE *stream, struct token *token)
{
    fprintf (stream, "%-7s %3zux%3zu %s\n", TYPES[token->type],
            token->line + 1, token->col + 1, token->value);
}

int
token_is_v (struct token *token, const char *value)
{
  if (!token) return 0;
  return !strcmp (token->value, value);
}

int
token_is_t (struct token *token, int type)
{
  if (!token) return 0;
  return token->type == type;
}

int
token_is (struct token *token, int type, const char *value)
{
  if (!token) return 0;
  return (token->type == type) && !strcmp (token->value, value);
}
