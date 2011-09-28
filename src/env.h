/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _ENV_H
#define _ENV_H 1

/* Compilation environment. This holds information about compile options and
 * environment settings. */

struct env {
    /* 32 or 64 */
    int bits;

    /* Whether to output debug information */
    int debug;

    /* Whether to use bounds-checking */
    int boundck;

    /* Whether to return null on OOM */
    int nulloom;

    /* malloc() function */
    char const *malloc;

    /* free() function */
    char const *free;

    /* Various warnings */
    int w_octalish;

    /* Paths */
    char const *crt1_32, *crti_32, *crtn_32, *ldso_32, *runtime_32,
         *crt1_64, *crti_64, *crtn_64, *ldso_64, *runtime_64,
         *crt1, *crti, *crtn, *ldso, *runtime,
         *llc, *llvm_as, *as, *ld;
};

#endif /* _ENV_H */
