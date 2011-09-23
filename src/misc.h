/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _MISC_H
#define _MISC_H 1

#ifndef HAVE_STRLCPY

#include <string.h>

/* strlcpy() is a nice function, even if the mighty Drepper doesn't agree.
 * Copy source into dest (which is of size sz), SAFELY. The string will
 * always be NUL-terminated and will never overflow. Returns the FULL size
 * of source, which can be checked for truncation. */
size_t strlcpy (char *dest, const char *source, size_t sz);

#endif /* HAVE_STRLCPY */

#endif /* _MISC_H */
