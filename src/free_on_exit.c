/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

/* See free_on_exit.h for an explanation */

#ifdef DEBUG

#include "free_on_exit.h"
#include "error.h"

#include <stdlib.h>

struct free_list {
    void *mem;
    struct free_list *next;
};

static struct free_list *list = NULL;

void free_on_exit (void *mem) {
    struct free_list *item = malloc (sizeof (*item));
    if (!item) error_errno ();
    item->mem = mem;
    item->next = list;
    list = item;
}

void do_free_on_exit (void) {
    struct free_list *hold;

    while (list) {
        hold = list;
        free (list->mem);
        list = list->next;
        free (hold);
    }
}

#endif /* DEBUG */
