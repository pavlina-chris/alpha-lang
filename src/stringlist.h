/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _STRINGLIST_H
#define _STRINGLIST_H 1

#include <stddef.h>

/* This is a mutable list of strings. It can be easily converted to an array,
 * or left as a list. */

struct stringlist {
    char **list;
    size_t n_in_list;
    size_t buf_size;
};

/* Initialise a string list. Returns nonzero on error. */
int
stringlist_init (struct stringlist *sl);

/* Append a string to the list. Takes a copy. Returns nonzero on error. */
int
stringlist_append (struct stringlist *sl, const char *s);

/* Get a string from the list. No bounds-checking. */
char *
stringlist_get (struct stringlist *sl, size_t i);

/* Set a string in the list. The index must exist. Returns nonzero on error. */
int
stringlist_set (struct stringlist *sl, size_t i, const char *s);

/* Get the length of the list. */
size_t
stringlist_len (struct stringlist *sl);

/* Get the stringlist as an array. When done with the array, call
 * stringlist_free() on the stringlist. */
char **
stringlist_array (struct stringlist *sl);

/* Free the stringlist */
void
stringlist_free (struct stringlist *sl);

#endif /* _STRINGLIST_H */
