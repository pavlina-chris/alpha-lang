/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

/* This code is for error reporting. Yes, it uses a global variable - this is
 * the only place in AlCo which does that. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "lex/lex.h"

/* Name of the running program (argv[0]) */
static char const *name;

void
error_set_name (char const *n) {
    name = n;
}

void
error_errno () {
    perror (name);
    exit (1);
}

void
error_message (const char *fmt, ...)
{
    fputs (name, stderr);
    fputs (": error: ", stderr);
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);
    exit (1);
}


void
warning_message (const char *fmt, ...) {
    fputs (name, stderr);
    fputs (": warning: ", stderr);
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);
}

/* Print out line number 'line' from the lexer (zero-based), with a caret at
 * 'col' and an underline running from 'start' to just before 'stop'. */
static void
annotate_line (struct lex *lex, size_t line, size_t col, size_t start,
               size_t stop)
{
    char *L = lex->lines[line];
    size_t i;

    /* Print line */
    for (i = 0; L[i] && L[i] != '\n'; ++i)
        fputc (L[i], stderr);
    fputc ('\n', stderr);

    /* Annotate */
    for (i = 0; L[i] && L[i] != '\n'; ++i) {
        if (i == col)
            fputc ('^', stderr);
        else if (i >= start && i < stop && L[i] != '\t')
            fputc ('~', stderr);
        else if (L[i] == '\t')
            fputc ('\t', stderr);
        else
            fputc (' ', stderr);
    }
    fputc ('\n', stderr);
}

void
cerror_at (struct lex *lex, struct token *tok, const char *fmt, ...)
{
    // Basic prefix: file:line:col: error:
    fprintf (stderr, "%s:%zu:%zu: error: ", lex->file,
            tok->line + 1, tok->col + 1);

    // Custom message
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);

    annotate_line (lex, tok->line, tok->col, tok->col,
                   tok->col + strlen (tok->value));

    exit (1);
}

void
cerror_after (struct lex *lex, struct token *tok, const char *fmt, ...)
{
    // Basic prefix: file:line:col: error:
    fprintf (stderr, "%s:%zu:%zu: error: ", lex->file,
            tok->line + 1, tok->col + 1);

    // Custom message
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);

    annotate_line (lex, tok->line, tok->col, 0, 0);

    exit (1);
}

void
cwarning_at (struct lex *lex, struct token *tok, const char *fmt, ...)
{
    // Basic prefix: file:line:col: warning:
    fprintf (stderr, "%s:%zu:%zu: warning: ", lex->file,
            tok->line + 1, tok->col + 1);

    // Custom message
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);

    annotate_line (lex, tok->line, tok->col, tok->col,
                   tok->col + strlen (tok->value));
}

void
cwarning_after (struct lex *lex, struct token *tok, const char *fmt, ...)
{
    // Basic prefix: file:line:col: error:
    fprintf (stderr, "%s:%zu:%zu: warning: ", lex->file,
            tok->line + 1, tok->col + 1);

    // Custom message
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);

    annotate_line (lex, tok->line, tok->col, 0, 0);
}

void
cerror_eof (struct lex *lex, const char *fmt, ...)
{
    fprintf (stderr, "%s: unexpected end of file: ", lex->file);

    // Custome message
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fputc ('\n', stderr);

    exit (1);
}
