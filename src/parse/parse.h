/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _PARSE_PARSE_H
#define _PARSE_PARSE_H 1

#include "../lex/lex.h"
#include "../env.h"
#include <stdio.h>

/* AST item types */
enum ast_tag {
  AST_FILE, AST_CLASS, AST_FUNCTION, AST_ST_BREAK, AST_ST_CONTINUE,
  AST_ST_VARDECL, AST_ST_NEW, AST_ST_DELETE, AST_ST_DO_WHILE, AST_ST_WHILE,
  AST_ST_FOR, AST_ST_IF, AST_ST_RETURN, AST_EXPR, AST_SCOPE
};

struct file {
  char const *name;
  int is_executable;
};

struct class {};
struct function {};
struct st_break {};
struct st_continue {};
struct st_vardecl {};
struct st_new {};
struct st_delete {};
struct st_do_while {};
struct st_while {};
struct st_for {};
struct st_if {};
struct st_return {};
struct expr {};
struct scope {};

union ast_union {
  struct file file;
  struct class class;
  struct function function;
  struct st_break st_break;
  struct st_continue st_continue;
  struct st_vardecl st_vardecl;
  struct st_new st_new;
  struct st_delete st_delete;
  struct st_do_while st_do_while;
  struct st_while st_while;
  struct st_for st_for;
  struct st_if st_if;
  struct st_return st_return;
  struct expr expr;
  struct scope scope;
};

/* Main AST structure */
struct ast {
  enum ast_tag tag;
  union ast_union o;
  struct token *token;
  struct ast **children, *parent;
  size_t n_children;
  size_t children_mem;
};

/* Create a 'struct ast' with its 'children' list initialised. Exit on error */
struct ast *new_ast (enum ast_tag type);

/* Pretty-print a 'struct ast' recursively. */
void print_ast (struct ast *ast, FILE *dest);
void _print_ast (struct ast *ast, FILE *dest, int indent);

/* Free a 'struct ast' recursively. */
void free_ast (struct ast *ast);

/* Add a child to an AST item. Exit on error */
void ast_add (struct ast *parent, struct ast *child);

/* Various AST parsers. Use parse_file to parse the entire file recursively. */
struct ast *parse_file (struct lex *lex, struct env *env);

/* Various AST printers. Just call print_ast(); this will choose the proper one.
 * When writing these, note: always indent 'indent' spaces, and do NOT append a
 * final newline (sometimes a parent printer may want to finish out your line
 * with a closing paren, etc)
 */
void print_file (struct ast *ast, FILE *dest, int indent);
void print_class (struct ast *ast, FILE *dest, int indent);
void print_function (struct ast *ast, FILE *dest, int indent);
void print_st_break (struct ast *ast, FILE *dest, int indent);
void print_st_continue (struct ast *ast, FILE *dest, int indent);
void print_st_vardecl (struct ast *ast, FILE *dest, int indent);
void print_st_new (struct ast *ast, FILE *dest, int indent);
void print_st_delete (struct ast *ast, FILE *dest, int indent);
void print_st_do_while (struct ast *ast, FILE *dest, int indent);
void print_st_while (struct ast *ast, FILE *dest, int indent);
void print_st_for (struct ast *ast, FILE *dest, int indent);
void print_st_if (struct ast *ast, FILE *dest, int indent);
void print_st_return (struct ast *ast, FILE *dest, int indent);
void print_st_expr (struct ast *ast, FILE *dest, int indent);
void print_st_scope (struct ast *ast, FILE *dest, int indent);

/* Various AST freers. Just call free_ast(); this will choose the proper one.
 * These do not free the entire struct ast. */
void free_file (struct ast *ast);
void free_class (struct ast *ast);
void free_function (struct ast *ast);
void free_st_break (struct ast *ast);
void free_st_continue (struct ast *ast);
void free_st_vardecl (struct ast *ast);
void free_st_new (struct ast *ast);
void free_st_delete (struct ast *ast);
void free_st_do_while (struct ast *ast);
void free_st_while (struct ast *ast);
void free_st_for (struct ast *ast);
void free_st_if (struct ast *ast);
void free_st_return (struct ast *ast);
void free_expr (struct ast *ast);
void free_scope (struct ast *ast);

#endif /* _PARSE_PARSE_H */
