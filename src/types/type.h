/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _TYPES_TYPE_H
#define _TYPES_TYPE_H 1

#include "../env.h"
#include "../lex/lex.h"

#define TYPE_NAME_MAX 255

/* Alpha type encodings */
enum type_encoding {
  UINT, SINT, BOOL, FLOAT, ARRAY, POINTER, OBJECT, NULLT
};

struct type {
  enum type_encoding enc;

  /* Size in bytes of the value - for example, u32 would have UINT type,
   * 4 size */
  int size;

  int is_const;
  int is_volatile;

  /* Used to link together complex types. For example:
   * int*[] : ARRAY -(child)-> POINTER -(child)-> i32
   * map<string, int> : OBJECT[map] -(child)-> OBJECT[string] -(sibling)-> i32
   */
  struct type *child_type, *sibling_type;

  /* Type name */
  char name[TYPE_NAME_MAX + 1];

  /* Whether the type was malloc()ed - used by type_free () */
  int was_malloced;
};

/* Some standard types to avoid constantly allocating new ones */
struct type *ty_i8;
struct type *ty_i16;
struct type *ty_i32;
struct type *ty_i64;
struct type *ty_u8;
struct type *ty_u16;
struct type *ty_u32;
struct type *ty_u64;
struct type *ty_f16;
struct type *ty_f32;
struct type *ty_f64;
struct type *ty_bool;
struct type *ty_null;

/* Get the 'size' and 'ssize' types */
struct type *get_ty_ssize (struct env *env);
struct type *get_ty_size (struct env *env);

/* Parse a type. Exit on error. */
struct type *parse_type (struct lex *lex, struct env *env);

/* Get type T* for a given type T. Exit on error. */
struct type *get_ty_pointer (struct type *T, struct env *env);

/* Get type T[] for a given type T. Exit on error. */
struct type *get_ty_array (struct type *T, struct env *env);

/* Make a copy, recursively, of type T:
 * _const: -1: not const, 0: same constness, 1: const
 * _volatile: -1: not volatile, 0: same volatility, 1: volatile
 */
struct type *copy_ty (struct type *T, int _const, int _volatile);

/* Free a type, recursively, if necessary. */
void type_free (struct type *T);

#endif /* _TYPES_TYPE_H */
