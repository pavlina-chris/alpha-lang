/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "filesystem.h"

#if defined (HAVE_UNISTD_H) && defined (HAVE_ACCESS)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

size_t size_of (const char *path)
{
  struct stat sbuf;
  if (stat (path, &sbuf)) return 0;
  return sbuf.st_size;
}

size_t size_of_f (FILE *f)
{
  int fd;
  struct stat sbuf;

  fd = fileno (f);
  if (fstat (fd, &sbuf)) return 0;
  return sbuf.st_size;
}

#else
#error "Bad platform - don't know how to define filesystem functions"


#endif /* Platforms */

