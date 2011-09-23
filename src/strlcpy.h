/* No copyright claimed. Header for code taken from FreeBSD libc - see
 * strlcpy.c */

#ifndef _STRLCPY_H
#define _STRLCPY_H 1

#include "config.h"

#ifndef HAVE_STRLCPY

#include <string.h>

/* strlcpy() is a nice function, even if The Drepper doesn't agree.
 * Copy source into dest (which is of size sz), SAFELY. The string will
 * always be NUL-terminated and will never overflow. Returns the FULL size
 * of source, which can be checked for truncation. */
size_t strlcpy (char *dest, const char *source, size_t sz);

#endif /* HAVE_STRLCPY */

#endif /* _STRLCPY_H */
