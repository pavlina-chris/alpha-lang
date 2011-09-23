/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _KEYWORDS_H
#define _KEYWORDS_H 1

/* Array of all keywords, excluding special type names. Ends in NULL */
const char **KEYWORDS;

/* Array of all special type names. Ends in NULL */
const char **TYPES;

/* Check whether a words is a keyword.
 * include_types: Whether to include special type names as "keywords"
 */
int is_keyword (const char *word, int include_types);

#endif /* _KEYWORDS_H */
