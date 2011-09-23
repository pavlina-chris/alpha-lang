/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _COLLECTOR_H
#define _COLLECTOR_H 1

/* String collector - efficiently collect a string one character at a time,
 * then make a big copy at the end. */

#include <string.h>

struct collector {
    char *buffer;
    size_t buffer_len;
    size_t buffer_used;
};

void coll_init (struct collector *coll);

/* Does not free the struct itself */
void coll_free (struct collector *coll);

/* Add a character - complain and exit on error */
void coll_append (struct collector *coll, char ch);

/* Add a whole string - complain and exit on error */
void coll_append_s (struct collector *coll, const char *s);

/* Get the string - complain and exit on error */
char *coll_get (struct collector *coll);

/* Get the string length */
size_t coll_len (struct collector *coll);

/* Clear the collector */
void coll_clear (struct collector *coll);

#endif /* _COLLECTOR_H */
