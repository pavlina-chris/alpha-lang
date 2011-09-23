/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "type.h"
#include "../error.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static struct type _ty_i8   = {SINT,  1, 0, 0, NULL, NULL, "i8", 0};
static struct type _ty_i16  = {SINT,  2, 0, 0, NULL, NULL, "i16", 0};
static struct type _ty_i32  = {SINT,  4, 0, 0, NULL, NULL, "int", 0};
static struct type _ty_i64  = {SINT,  8, 0, 0, NULL, NULL, "i64", 0};
static struct type _ty_u8   = {SINT,  1, 0, 0, NULL, NULL, "u8", 0};
static struct type _ty_u16  = {SINT,  2, 0, 0, NULL, NULL, "u16", 0};
static struct type _ty_u32  = {SINT,  4, 0, 0, NULL, NULL, "unsigned", 0};
static struct type _ty_u64  = {SINT,  8, 0, 0, NULL, NULL, "u64", 0};
static struct type _ty_f16  = {FLOAT, 2, 0, 0, NULL, NULL, "f16", 0};
static struct type _ty_f32  = {FLOAT, 4, 0, 0, NULL, NULL, "float", 0};
static struct type _ty_f64  = {FLOAT, 8, 0, 0, NULL, NULL, "double", 0};
static struct type _ty_bool = {BOOL,  1, 0, 0, NULL, NULL, "bool", 0};
static struct type _ty_null = {NULLT, 1, 0, 0, NULL, NULL, "#null#", 0};

static struct type _ty_ssize_32 = {SINT, 4, 0, 0, NULL, NULL, "ssize", 0};
static struct type _ty_ssize_64 = {SINT, 8, 0, 0, NULL, NULL, "ssize", 0};
static struct type _ty_size_32  = {UINT, 4, 0, 0, NULL, NULL, "size", 0};
static struct type _ty_size_64  = {UINT, 8, 0, 0, NULL, NULL, "size", 0};

struct type *ty_i8   = &_ty_i8;
struct type *ty_i16  = &_ty_i16;
struct type *ty_i32  = &_ty_i32;
struct type *ty_i64  = &_ty_i64;
struct type *ty_u8   = &_ty_u8;
struct type *ty_u16  = &_ty_u16;
struct type *ty_u32  = &_ty_u32;
struct type *ty_u64  = &_ty_u64;
struct type *ty_f16  = &_ty_f16;
struct type *ty_f32  = &_ty_f32;
struct type *ty_f64  = &_ty_f64;
struct type *ty_bool = &_ty_bool;
struct type *ty_null = &_ty_null;

struct type *get_ty_ssize (struct env *env)
{
  assert (env->bits == 32 || env->bits == 64);
  return env->bits == 32 ? &_ty_ssize_32 : &_ty_ssize_64;
}

struct type *get_ty_size (struct env *env)
{
  assert (env->bits == 32 || env->bits == 64);
  return env->bits == 32 ? &_ty_size_32 : &_ty_size_64;
}

struct type *get_ty_pointer (struct type *T, struct env *env)
{
  struct type *ty_ptr = malloc (sizeof (*ty_ptr));
  if (!ty_ptr) error_errno ();
  ty_ptr->enc = POINTER;
  ty_ptr->size = env->bits / 8;
  ty_ptr->is_const = 0;
  ty_ptr->is_volatile = 0;
  ty_ptr->child_type = T;
  ty_ptr->sibling_type = NULL;
  ty_ptr->was_malloced = 1;

  /* Copy T->name with an asterisk appended */
  size_t name_len = strlen (T->name);
  if (name_len + 1 > TYPE_NAME_MAX) {
    error_message ("internal error: type name '%s*' is "
                   "too long for name buffer - sorry...", T->name);
  }
  
  memcpy (ty_ptr->name, T->name, name_len);
  ty_ptr->name[name_len] = '*';
  ty_ptr->name[name_len + 1] = 0;

  return ty_ptr;
}

struct type *get_ty_array (struct type *T, struct env *env)
{
  struct type *ty_arr = malloc (sizeof (*ty_arr));
  if (!ty_arr) error_errno ();
  ty_arr->enc = ARRAY;
  ty_arr->size = env->bits / 8;
  ty_arr->is_const = 0;
  ty_arr->is_volatile = 0;
  ty_arr->child_type = T;
  ty_arr->sibling_type = NULL;
  ty_arr->was_malloced = 1;

  /* Copy T->name with [] appended */
  size_t name_len = strlen (T->name);
  if (name_len + 2 > TYPE_NAME_MAX) {
    error_message ("internal error: type name '%s[]' is too long for name "
                   "buffer - sorry...", T->name);
  }

  memcpy (ty_arr->name, T->name, name_len);
  ty_arr->name[name_len] = '[';
  ty_arr->name[name_len + 1] = ']';
  ty_arr->name[name_len + 2] = 0;

  return ty_arr;
}

/* See below for description */
static struct type *_copy_ty (struct type *T, int follow_sibs)
{
  struct type *new_ty;
  if (!T) return NULL;

  new_ty = malloc (sizeof (*new_ty));
  if (!new_ty) error_errno ();

  /* Most items can be directly copied */
  memcpy (new_ty, T, sizeof (*new_ty));

  new_ty->was_malloced = 1;
  new_ty->child_type = _copy_ty (new_ty->child_type, 1);
  if (follow_sibs)
    new_ty->sibling_type = _copy_ty (new_ty->sibling_type, 1);
  else
    new_ty->sibling_type = NULL;

  return new_ty;
}

struct type *copy_ty (struct type *T, int _const, int _volatile)
{
  /* Recursively copy T through its child types. We don't want to follow its
   * siblings, but we DO want to follow its childrens' siblings, so _copy_ty
   * allows this choice. */
  struct type *new_ty = _copy_ty (T, 0);

  if (_const < 0)
    new_ty->is_const = 0;
  else if (_const > 0)
    new_ty->is_const = 1;

  if (_volatile < 0)
    new_ty->is_volatile = 0;
  else if (_volatile > 0)
    new_ty->is_volatile = 1;

  return new_ty;
}
