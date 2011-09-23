/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "keywords.h"
#include <string.h>

const char *_KEYWORDS[] = {
  "class", "extern", "macro"
  "let", "const", "static", "volatile", "threadlocal", "nomangle",
  "null", "allowconflict", "global",
  "record", "switch", "case", "default", "if", "else", "for",
  "foreach", "do", "while", "return", "as", "true", "false",
  NULL};

const char *_TYPES[] = {
  "i8", "i16", "i32", "i64", "ssize", "int",
  "u8", "u16", "u32", "u64", "size", "unsigned",
  "float", "double", "var", "void", "bool",
  NULL};

const char **KEYWORDS = &_KEYWORDS[0];
const char **TYPES = &_TYPES[0];

int is_keyword (const char *word, int include_types)
{
  size_t i;
  for (i = 0; KEYWORDS[i]; ++i) {
    if (!strcmp (KEYWORDS[i], word))
      return 1;
  }
  if (include_types) {
    for (i = 0; TYPES[i]; ++i) {
      if (!strcmp (TYPES[i], word))
        return 1;
    }
  }
  return 0;
}
