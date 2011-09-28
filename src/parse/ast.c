/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "parse.h"
#include "../error.h"
#include <stdlib.h>
#include <string.h>

static const size_t DEFAULT_CHILDREN_MEM[] = {
  /* file */         16,
    /* class */      16,
    /* function */    1, /* single child - the scope */
    /* st_break */    0,
    /* st_continue */ 0,
    /* st_vardecl */  1, /* usually one variable per declaration */
    /* st_new */      2,
    /* st_delete */   1,
    /* st_do_while */ 2, /* condition and scope */
    /* st_while */    2, /* condition and scope */
    /* st_for */      4, /* initialiser, condition, increment, scope */
    /* st_if */       3, /* condition, true, false */
    /* st_return */   1,
    /* expr */        2,
    /* scope */      16 };

struct ast *
new_ast (enum ast_tag type)
{
  struct ast *s;

  s = malloc (sizeof (*s));
  if (!s) error_errno ();
  memset (s, 0, sizeof (*s));

  s->tag = type;
  s->children = malloc (DEFAULT_CHILDREN_MEM[type] * sizeof (*s->children));
  if (!s->children) {
    free (s);
    error_errno ();
  }
  
  s->n_children = 0;
  s->children_mem = DEFAULT_CHILDREN_MEM[type];

  return s;
}

void
ast_add (struct ast *parent, struct ast *child)
{
  if (parent->n_children + 1 > parent->children_mem) {
    struct ast **new_children;
    size_t new_sz = (2 * parent->children_mem) * sizeof (*parent->children);
    new_children = realloc (parent->children, new_sz);
    if (!new_children) error_errno ();
    parent->children = new_children;
    parent->children_mem *= 2;
  }
  parent->children[parent->n_children] = child;
  ++parent->n_children;
  child->parent = parent;
}

void
free_ast (struct ast *ast)
{
  void (*free_fns[]) (struct ast *) =
    { free_file, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL };
  size_t i;

  free_fns[ast->tag] (ast);
  for (i = 0; i < ast->n_children; ++i) {
    free_ast (ast->children[i]);
  }
  free (ast);
}

void
print_ast (struct ast *ast, FILE *dest)
{
  _print_ast (ast, dest, 0);
  fputc ('\n', dest);
}

void
_print_ast (struct ast *ast, FILE *dest, int indent)
{
  void (*print_fns[]) (struct ast *, FILE *, int) =
    { print_file, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL };
  
  print_fns[ast->tag] (ast, dest, indent);
}
