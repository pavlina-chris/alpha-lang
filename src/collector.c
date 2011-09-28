/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "collector.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>

#define DEFAULT_CAPACITY 16

static void
ensure_space (struct collector *coll, size_t n)
{
    size_t target_sz;
    char *new_buffer;

    /* Calculate the new size of the buffer. I apologize for the loop... */
    target_sz = coll->buffer_len;
    while (n + coll->buffer_used > target_sz) {
        target_sz *= 2;
    }

    /* Reallocate */
    new_buffer = realloc (coll->buffer, target_sz);
    if (!new_buffer) error_errno ();
    coll->buffer = new_buffer;
    coll->buffer_len = target_sz;

}

void
coll_init (struct collector *coll)
{
    coll->buffer_len = DEFAULT_CAPACITY;
    coll->buffer_used = 0;
    coll->buffer = malloc (DEFAULT_CAPACITY);
    if (!coll->buffer)
        error_errno ();
}

void
coll_free (struct collector *coll)
{
    free (coll->buffer);
}

void
coll_append (struct collector *coll, char ch)
{
    ensure_space (coll, 1);
    coll->buffer[coll->buffer_used] = ch;
    ++coll->buffer_used;
}

void
coll_append_s (struct collector *coll, const char *s)
{
    size_t len;

    len = strlen (s);
    ensure_space (coll, len);
    memcpy (coll->buffer + coll->buffer_used, s, len);
    coll->buffer_used += len;
}

char *
coll_get (struct collector *coll)
{
    char *s;

    s = malloc (coll->buffer_used + 1);
    if (!s) error_errno ();
    memcpy (s, coll->buffer, coll->buffer_used);
    s[coll->buffer_used] = 0;
    return s;
}

size_t
coll_len (struct collector *coll)
{
    return coll->buffer_used;
}

void
coll_clear (struct collector *coll)
{
    coll->buffer_used = 0;
}
