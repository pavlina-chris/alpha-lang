/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "stringlist.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 16

/* Make room for another entry in a stringlist. */
static int make_room (struct stringlist *sl)
{
    if (sl->n_in_list >= (sl->buf_size - 1)) {
        char **new_list = realloc (sl->list,
                2 * sl->buf_size * sizeof (*new_list));
        if (!new_list) return 1;
        memset (new_list + sl->buf_size, 0, sl->buf_size * sizeof (*new_list));
        sl->list = new_list;
        sl->buf_size *= 2;
    }
    return 0;
}

int stringlist_init (struct stringlist *sl)
{
    sl->list = malloc (DEFAULT_SIZE * sizeof (*sl->list));
    if (!sl->list) return 1;
    memset (sl->list, 0, DEFAULT_SIZE * sizeof (*sl->list));
    sl->n_in_list = 0;
    sl->buf_size = DEFAULT_SIZE;
    return 0;
}

int stringlist_append (struct stringlist *sl, const char *s)
{
    char *copy = strdup (s);
    if (!copy) return 1;

    if (make_room (sl)) return 1;
    sl->list[sl->n_in_list] = copy;
    ++sl->n_in_list;

    return 0;
}

char *stringlist_get (struct stringlist *sl, size_t i)
{
    return sl->list[i];
}

int stringlist_set (struct stringlist *sl, size_t i, const char *s)
{
    char *copy = strdup (s);
    if (!copy) return 1;

    if (sl->list[i]) free (sl->list[i]);
    sl->list[i] = copy;

    return 0;
}

size_t stringlist_len (struct stringlist *sl)
{
    return sl->n_in_list;
}

char **stringlist_array (struct stringlist *sl)
{
    return sl->list;
}

void stringlist_free (struct stringlist *sl)
{
    size_t i;
    for (i = 0; i < sl->n_in_list; ++i) {
        if (sl->list[i]) free (sl->list[i]);
    }
    free (sl->list);
}
