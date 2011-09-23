/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _FREE_ON_EXIT_H
#define _FREE_ON_EXIT_H 1

/* This file is for debug purposes. There are some places where I have chosen
 * not to free() after malloc(), for efficiency reasons. When compiling a
 * program, the compiler is usually run separately many times. There is some
 * memory that is allocated right at init, and kept until the end. When the
 * program exits, the OS will very quickly reclaim all of its memory, so I
 * don't free() those bits. Normally, these functions are NOPs, but if DEBUG
 * is defined, they will track allocated memory. Then do_free_on_exit() will
 * clean it all up. This helps to clean up valgrind leak reports, if checking
 * for real leaks. */

#ifdef DEBUG
void free_on_exit (void *mem);
void do_free_on_exit (void);
#else
#define free_on_exit(m) ((void) 0)
#define do_free_on_exit() ((void) 0)
#endif /* DEBUG */

#endif /* _FREE_ON_EXIT_H */
