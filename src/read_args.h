/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _READ_ARGS_H
#define _READ_ARGS_H 1

struct args;

/* Allow this many of any list-type argument */
#define LIST_ARG_MAX 256

/* Parse command line arguments into a struct args
 * args: struct args* into which the arguments go
 * argc: argc from main()
 * argv: argv form main()
 * return: nonzero if main() should return args->exit_code
 */
int read_args (struct args *args, int argc, char **argv);

/* This structure holds command line argument information after parsing.
 * Note: Remember that since only one of these will ever be used in the entire
 * run of the program, even though some fields may be malloc()ed, they need not
 * be free()d. This will never cause a leak. */
struct args {

    /* If read_args() decides that the compiler should exit immediately, the
     * exit code will be here. Undefined otherwise. */
    int exit_code;

    /* Verbose? */
    int verbose;

    /* Run in debug mode? */
    int debug_mode;

    /* Call abort() on compiler error? */
    int error_trace;

    /* Dump paths and quit? */
    int paths_only;

    /* Dump the token list and quit? */
    int tokens_only;

    /* Dump the AST after parsing and quit? */
    int ast_only;

    /* Dump the AST before type checking and quit? */
    int pre_ast_only;

    /* Force compiling on an unsupported platform? */
    int force_platform;

    /* Desired output file, or NULL */
    char const *output;

    /* List of source files, or NULL */
    char const **sources;

    /* Output debugging information? */
    int debug;

    /* Optimisation level */
    int optlevel;

    /* Machine ID string */
    char const *machine;

    /* Generate position-independent code? */
    int fpic;

    /* List of libraries to link with, followed by NULL */
    char const **libs;

    /* List of library search paths, followed by NULL */
    char const **lib_dirs;

    /* List of package search paths, followed by NULL */
    char const **pkg_dirs;

    /* List of extra options to be given to the LLVM static compiler, followed
     * by NULL */
    char const **llc_opts;

    /* List of extra options to be given to the assembler, followed by NULL */
    char const **as_opts;

    /* List of extra options to be given to the linker, followed by NULL */
    char const **ld_opts;

    /* Emit LLVM formats rather than machine code? */
    int emit_llvm;

    /* Stop after generating assembly? */
    int assembly;

    /* Stop after assembling? */
    int objfile;

    /* Do not use garbage collection? */
    int nogc;

    /* Do not abort on OOM? */
    int nomemabort;

    /* Do not use bounds-checking */
    int noboundck;

    /* Systems mode? */
    int sm;

    /* Function to use for malloc() */
    char const *malloc;

    /* Function to use for free() */
    char const *free;

    /* Paths */
    char const *crt1_32, *crti_32, *crtn_32, *ldso_32, *runtime_32,
         *crt1_64, *crti_64, *crtn_64, *ldso_64, *runtime_64,
         *llc, *llvm_as, *as, *ld;
};

#endif /* _READ_ARGS_H */
