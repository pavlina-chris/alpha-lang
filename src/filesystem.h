/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H 1

#include "config.h"

/* This header provides filesystem functions. They are used in place of the
 * OS-native ones; filesystem.c may use preprocessor directives to compile
 * the correct functions for the OS. */

/* int f_access (const char *path, int mode):
 * Alias to or emulation of POSIX's access() - see access(2). This header will
 * bring in or provide R_OK, W_OK, X_OK, F_OK. */

/* size_t size_of (const char *path):
 * Return the size of the file in bytes. Returns 0 with errno set if the file
 * cannot be accessed. */

/* size_t size_of_f (FILE *f):
 * Return the size of the open file in bytes. Returns 0 with errno set if the
 * file cannot be accessed. */

#if defined (HAVE_UNISTD_H) && defined (HAVE_ACCESS)
#include <unistd.h>
#include <stdio.h>

#define f_access access
size_t size_of (const char *path);
size_t size_of_f (FILE *f);

#else
#error "Bad platform - don't know how to define filesystem functions"

#endif /* Platforms */

#endif /* _FILESYSTEM_H */
