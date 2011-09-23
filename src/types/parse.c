/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "type.h"
#include "../lex/lex.h"
#include "../error.h"
#include "../strlcpy.h"
#include <stdlib.h>
#include <string.h>

/* This is the main type parser in AlCo. It can parse all type declarations.
 * Note one trick: If it sees a >> token, it will advance the (notably writable)
 * token to just > after consuming a single character. This is so that things
 * like map<int, list<string>> are parsed correctly. Yes, I know it's a dirty
 * trick. No, I won't change it.
 *
 * This function will always return a newly-allocated type, even for simple
 * primitives, so you can directly link or modify.
 */

/* Helper: Initialise 'enc' and 'size' from the type name. Only reads the 'name'
 * field - everything else may be uninitialised. */
static void
init_enc_size(struct type *type, struct env *env)
{
        /* Make sure these arrays line up. It is not necessary to match the NULL
         * in 'names' with the other arrays. */
        static const char *names[] = {
                "i8", "i16", "i32", "i64", "int",      "ssize",
                "u8", "u16", "u32", "u64", "unsigned", "size",
                      "f16", "f32", "f64", "float",    "double", "bool", NULL };
        const size_t sizes[] = {
                1,    2,     4,     8,     4,          env->bits / 8,
                1,    2,     4,     8,     4,          env->bits / 8,
                      2,     4,     8,     4,          8,        1 };
        static const enum type_encoding encodings[] = {
                SINT, SINT,  SINT,  SINT,  SINT,       SINT,
                UINT, UINT,  UINT,  UINT,  UINT,       UINT,
                      FLOAT, FLOAT, FLOAT, FLOAT,      FLOAT,    BOOL };

        size_t i;
        for (i = 0; names[i]; ++i) {
                if (!strcmp(type->name, names[i])) {
                        type->size = sizes[i];
                        type->enc = encodings[i];
                        return;
                }
        }

        /* If we get here, this is not a primitive. The only nonprimitive named
         * type is an object. */
        type->size = env->bits / 8;
        type->enc = OBJECT;
}

static void
do_base_name(struct type *t, struct lex *lex, struct env *env)
{
        struct token *token;
        size_t n;

        /* Base name */
        token = lexer_next(lex);
        if (!token)
                cerror_eof(lex, "expected type name");
        else if (!token_is_t(token, T_WORD))
                cerror_at(lex, token, "expected type name");
        n = strlcpy(t->name, token->value, TYPE_NAME_MAX);
        if (n >= TYPE_NAME_MAX) {
                cerror_at(lex, token,
                          "type name too long - maximum length is %zu",
                          TYPE_NAME_MAX - 1);
        }

        /* From base name we can get much information */
        init_enc_size(t, env);
}

static void
do_arguments(struct type *t, struct lex *lex, struct env *env)
{
        struct type *temp, *last_argument;
        struct token *args_token, *token;

        /* Arguments */
        args_token = lexer_peek(lex);
        if (!token_is(args_token, T_OPER, "<"))
                return;

        if (t->enc != OBJECT) {
                cerror_at(lex, args_token,
                          "only object types may have arguments");
        }

        lexer_next(lex);
        t->child_type = last_argument = NULL;
        while (1) {
                temp = parse_type(lex, env);
                if (last_argument) {
                        last_argument->sibling_type = temp;
                } else {
                        t->child_type = last_argument = temp;
                }
                token = lexer_next(lex);
                if (token_is(token, T_OPER, ">")) {
                        break;
                } else if (token_is(token, T_OPER, ">>")) {
                        /* Rewrite >> to > */
                        token->value[1] = 0;
                        ++token->col;
                        break;
                } else if (!token) {
                        cerror_eof(lex, "expected , or >");
                } else if (!token_is(token, T_OPER, ",")) {
                        cerror_after(lex, lexer_last(lex),
                                     "expected ,");
                }
        }
}

static struct type *
do_modifiers(struct type *t, struct lex *lex, struct env *env)
{
        struct token *token;

        while (1) {
                token = lexer_peek(lex);
                if (token_is(token, T_OPER, "*")) {
                        t = get_ty_pointer (t, env);
                        lexer_next(lex);

                } else if (token_is(token, T_OPER, "[")) {
                        lexer_next(lex);
                        struct token *token2 = lexer_next(lex);
                        if (!token2)
                                cerror_eof(lex, "expected ]");
                        else if (!token_is(token2, T_OPER, "]"))
                                cerror_after(lex, token, "expected ]");
                        t = get_ty_array(t, env);

                } else if (token_is(token, T_WORD, "const")) {
                        t->is_const = 1;
                        lexer_next(lex);
                } else if (token_is(token, T_WORD, "volatile")) {
                        t->is_volatile = 1;
                        lexer_next(lex);
                } else
                        break;
        }
        return t;
}

struct type *
parse_type(struct lex *lex, struct env *env)
{
        struct type *t;

        t = malloc(sizeof (*t));
        if (!t) error_errno();

        do_base_name(t, lex, env);

        do_arguments(t, lex, env);

        t = do_modifiers(t, lex, env);
        
        return t;
}
