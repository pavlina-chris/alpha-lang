/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "default_paths.h"
#include "read_args.h"
#include "error.h"
#include "env.h"
#include "config_file.h"
#include "filesystem.h"
#include "lex/lex.h"
#include "parse/parse.h"
#include "free_on_exit.h"

static void
construct_env (struct args *args, struct env *env)
{
    assert (args && env);

    /* Quick check: "32" or "64" */
    assert (!strcmp (args->machine, "32") || !strcmp (args->machine, "64"));
    env->bits = (args->machine[0] == '3' ? 32 : 64);

    if (args->nogc) {
        env->malloc = "malloc";
        env->free = "free";
    }
    else {
        env->malloc = "GC_malloc";
        env->free = "GC_free";
    }

    if (args->sm) {
        env->malloc = "malloc";
        env->free = "free";
        env->boundck = 0;
        env->nulloom = 1;
    }

    if (args->malloc && *args->malloc)
        env->malloc = args->malloc;

    if (args->free && *args->free)
        env->free = args->free;

    env->boundck = !args->noboundck;
    env->debug = args->debug;

    env->w_octalish = args->w_octalish;
}

static void
set_paths(struct args *args, struct env *env)
{
    struct config_file cfg;

    /* Set all defaults */
    env->crt1_32 = DEFAULT_CRT1_32;
    env->crti_32 = DEFAULT_CRTI_32;
    env->crtn_32 = DEFAULT_CRTN_32;
    env->ldso_32 = DEFAULT_LDSO_32;
    env->runtime_32 = DEFAULT_RUNTIME_32;
    env->crt1_64 = DEFAULT_CRT1_64;
    env->crti_64 = DEFAULT_CRTI_64;
    env->crtn_64 = DEFAULT_CRTN_64;
    env->ldso_64 = DEFAULT_LDSO_64;
    env->runtime_64 = DEFAULT_RUNTIME_64;
    env->llc = DEFAULT_LLC;
    env->llvm_as = DEFAULT_LLVM_AS;
    env->as = DEFAULT_AS;
    env->ld = DEFAULT_LD;

    /* Load from config file */
    load_config (&cfg);
    env->crt1_32 = cfg.crt1_32 ? cfg.crt1_32 : env->crt1_32;
    env->crti_32 = cfg.crti_32 ? cfg.crti_32 : env->crti_32;
    env->crtn_32 = cfg.crtn_32 ? cfg.crtn_32 : env->crtn_32;
    env->ldso_32 = cfg.ldso_32 ? cfg.ldso_32 : env->ldso_32;
    env->runtime_32 = cfg.runtime_32 ? cfg.runtime_32 : env->runtime_32;
    env->crt1_64 = cfg.crt1_64 ? cfg.crt1_64 : env->crt1_64;
    env->crti_64 = cfg.crti_64 ? cfg.crti_64 : env->crti_64;
    env->crtn_64 = cfg.crtn_64 ? cfg.crtn_64 : env->crtn_64;
    env->ldso_64 = cfg.ldso_64 ? cfg.ldso_64 : env->ldso_64;
    env->runtime_64 = cfg.runtime_64 ? cfg.runtime_64 : env->runtime_64;
    env->llc = cfg.llc ? cfg.llc : env->llc;
    env->llvm_as = cfg.llvm_as ? cfg.llvm_as : env->llvm_as;
    env->as = cfg.as ? cfg.as : env->as;
    env->ld = cfg.ld ? cfg.ld : env->ld;
        
    /* Load from arguments */
    env->crt1_32 = args->crt1_32 ? args->crt1_32 : env->crt1_32;
    env->crti_32 = args->crti_32 ? args->crti_32 : env->crti_32;
    env->crtn_32 = args->crtn_32 ? args->crtn_32 : env->crtn_32;
    env->ldso_32 = args->ldso_32 ? args->ldso_32 : env->ldso_32;
    env->runtime_32 = args->runtime_32 ? args->runtime_32 : env->runtime_32;
    env->crt1_64 = args->crt1_64 ? args->crt1_64 : env->crt1_64;
    env->crti_64 = args->crti_64 ? args->crti_64 : env->crti_64;
    env->crtn_64 = args->crtn_64 ? args->crtn_64 : env->crtn_64;
    env->ldso_64 = args->ldso_64 ? args->ldso_64 : env->ldso_64;
    env->runtime_64 = args->runtime_64 ? args->runtime_64 : env->runtime_64;
    env->llc = args->llc ? args->llc : env->llc;
    env->llvm_as = args->llvm_as ? args->llvm_as : env->llvm_as;
    env->as = args->as ? args->as : env->as;
    env->ld = args->ld ? args->ld : env->ld;

    if (env->bits == 32) {
        env->crt1 = env->crt1_32;
        env->crti = env->crti_32;
        env->crtn = env->crtn_32;
        env->ldso = env->ldso_32;
        env->runtime = env->runtime_32;
    } else {
        assert (env->bits == 64);
        env->crt1 = env->crt1_64;
        env->crti = env->crti_64;
        env->crtn = env->crtn_64;
        env->ldso = env->ldso_64;
        env->runtime = env->runtime_64;
    }
}

static void
check_paths(struct args *args, struct env *env)
{

#define TRY(a, fn) if (f_access (fn, a)) error_message ("cannot access %s", fn)

    TRY(X_OK, env->llc);
    TRY(X_OK, env->llvm_as);
    TRY(X_OK, env->as);
    TRY(X_OK, env->ld);

    if (args->emit_llvm || args->assembly || args->objfile)
        return;

    TRY(R_OK, env->crt1);
    TRY(R_OK, env->crti);
    TRY(R_OK, env->crtn);
    TRY(R_OK | X_OK, env->ldso);
    TRY(R_OK, env->runtime);
#undef TRY
}

/* Output one line of the path dump */
static inline void
dump_path(const char *name, const char *path)
{
    printf ("%-10s (%s): %s\n", name,
           f_access (path, F_OK) ? "PRESENT" : "MISSING", path);
}

static void
dump_paths(struct env *env)
{
    dump_path ("crt1-32", env->crt1_32);
    dump_path ("crti-32", env->crti_32);
    dump_path ("crtn-32", env->crtn_32);
    dump_path ("ldso-32", env->ldso_32);
    dump_path ("runtime-32", env->runtime_32);
    dump_path ("crt1-64", env->crt1_64);
    dump_path ("crti-64", env->crti_64);
    dump_path ("crtn-64", env->crtn_64);
    dump_path ("ldso-64", env->ldso_64);
    dump_path ("runtime-64", env->runtime_64);
    dump_path ("llc", env->llc);
    dump_path ("llvm-as", env->llvm_as);
    dump_path ("as", env->as);
    dump_path ("ld", env->ld);
}

int main
(int argc, char **argv)
{
    struct args args;
    struct env env;
    size_t i;
    /* List of booleans corresponding to sources: is this a .al file? */
    char al_files[LIST_ARG_MAX] = {0};

    /* Error handling code needs to know my name */
    error_set_name (argv[0]);

    /* Read arguments */
    if (read_args (&args, argc, argv))
        return args.exit_code;

    /* Set up for compilation */
    construct_env (&args, &env);
    set_paths (&args, &env);
    if (args.paths_only) {
        dump_paths (&env);
        return 0;
    }

    if (!args.sources[0]) {
        error_message ("no sources to compile");
    }

    /* Check if the string ends with '.al' or '.o' */
    for (i = 0; i < LIST_ARG_MAX; ++i) {
        if (!args.sources[i]) break;
        size_t len = strlen(args.sources[i]);
        if (len < 3) {
            /* strlen(".al") == 3 */
            error_message("unknown source type: %s",
                          args.sources[i]);
        }
        if (args.sources[i][len - 1] == 'l' &&
            args.sources[i][len - 2] == 'a' &&
            args.sources[i][len - 3] == '.' &&
            len >= 4) {
            /* .al file - this is OK */
            al_files[i] = 1;
        } else if (args.sources[i][len - 1] == 'o' &&
                   args.sources[i][len - 2] == '.') {
            /* .o file - this is OK */
        } else {
            error_message("unknown source type: %s",
                          args.sources[i]);
        }
    }
        
    check_paths(&args, &env);

    /* Compile */
    /* Run lex and parse on each file. */
    for (i = 0; i < LIST_ARG_MAX; ++i) {
        struct lex lex;
        if (!al_files[i]) continue;
        lexer_init(args.sources[i], &env, &lex);
        lexer_lex(&lex);

        if (args.tokens_only) {
            /* Option -tokens: dump tokens */
            size_t i;
            for (i = 0; i < lex.n_tokens; ++i)
                print_token(stdout, &lex.tokens[i]);
            lexer_free (&lex);
            do_free_on_exit ();
            return 0;
        }

        struct ast *ast = parse_file (&lex, &env);
        if (args.pre_ast_only) {
            print_ast (ast, stdout);
            free_ast (ast);
            lexer_free (&lex);
            do_free_on_exit ();
            return 0;
        }

        free_ast (ast);
        lexer_free (&lex);
    }

    do_free_on_exit ();

    return 0;
}

