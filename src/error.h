/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _MISC_H
#define _MISC_H 1

#include "lex/lex.h"

/* Store argv[0] for use in error messages */
void error_set_name (char const *);

/* Report an error based on errno, then exit. */
void error_errno ();

/* Report a general (not compile) error, then exit. printf() usage. */
void error_message (const char *fmt, ...);

/* Report a general (not compile) warning. printf() usage. */
void warning_message (const char *fmt, ...);

/* Report a compile error, then exit. printf() usage. */
void cerror_at (struct lex *lex, struct token *tok, const char *fmt, ...);

/* Report a compile warning. printf() usage. */
void cwarning_at (struct lex *lex, struct token *tok, const char *fmt, ...);

/* Report a compile error, then exit. printf() usage. */
void cerror_after (struct lex *lex, struct token *tok, const char *fmt, ...);

/* Report a compile warning. printf() usage. */
void cwarning_after (struct lex *lex, struct token *tok, const char *fmt, ...);

/* Report an unexpected-eof error, then exit. printf() usage. */
void cerror_eof (struct lex *lex, const char *fmt, ...);

#endif /* _MISC_H */
