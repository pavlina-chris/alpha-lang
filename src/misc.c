/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "misc.h"
#include <string.h>

#ifndef HAVE_STRLCPY
size_t strlcpy (char *dest, const char *source, size_t sz)
{
  /* Copy source into dest, stopping at the maximum length of dest (sz), always
   * adding a NUL. Return the full length of source */

  size_t i;

  for (i = 0; i < sz; ++i) {
    if (!source[i]) {
      dest[i] = 0;
      return i;
    }
    dest[i] = source[i];
  }

  /* If we're still here, there are more characters in source. Zero out the last
   * byte of dest, then continue counting. */
  dest[sz - 1] = 0;

  for (; source[i]; ++i) {}

  return i;
}
#endif /* HAVE_STRLCPY */
