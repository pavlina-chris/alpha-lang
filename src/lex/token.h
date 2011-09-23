/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _LEX_TOKEN_H
#define _LEX_TOKEN_H 1

#include <stdio.h>

#define T_STRING  1
#define T_WORD    2
#define T_INT     3
#define T_REAL    4
#define T_OPER    5
#define T_EXTRA   6
#define T_SPECIAL 7

struct token {
    size_t line, col;
    int type;
    char *value;
};

/* Print a token */
void print_token (FILE *stream, struct token *token);

/* Check if a token has a certain value. NULL-safe. */
int token_is_v (struct token *token, const char *value);

/* Check if a token has a certain type. NULL-safe. */
int token_is_t (struct token *token, int type);

/* Check if a token has a certain type and value. NULL-safe. */
int token_is (struct token *token, int type, const char *value);

#endif /* _LEX_TOKEN_H */
