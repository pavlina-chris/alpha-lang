/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "parse.h"
#include "../error.h"
#include "../keywords.h"

/* Read the "executable" or "package" declaration. */
static int read_exec_package (struct lex *lex)
{
  struct token *token = lexer_next (lex);
  if (!token) {
    error_message ("no code in source file %s", lex->file);
  } else if (token_is (token, T_WORD, "executable")) {
    return 1;
  } else if (token_is (token, T_WORD, "package")) {
    return 0;
  } else {
    cerror_at (lex, token, "expected 'package' or 'executable'");
  }
  return 0; // Shut up
}

/* Read the package name and semicolon */
static char *read_name (struct lex *lex)
{
  char *name;
  struct token *token = lexer_next (lex);
  if (!token) {
    cerror_eof (lex, "expected name");
  } else if (!token_is_t (token, T_WORD)) {
    cerror_at (lex, token, "expected name");
  } else if (is_keyword (token->value, 1)) {
    cerror_at (lex, token, "expected name");
  }
  name = token->value;

  token = lexer_next (lex);
  if (!token)
    cerror_eof (lex, "expected ;");
  else if (!token_is (token, T_OPER, ";"))
    cerror_after (lex, lexer_last (lex), "expected ;");

  return name;
}

static struct ast *read_child (struct lex *lex, struct env *env)
{
  struct token *token = lexer_peek (lex);

  if (!token) return NULL;

  /* All we support so far are functions and 'extern' declarations, which are
   * both handled by parse_function. */
  else
    return parse_function (lex, env);
}

struct ast *parse_file (struct lex *lex, struct env *env)
{
  struct ast *ast = new_ast (AST_FILE);
  
  /* Get the first token */
  ast->token = lexer_peek (lex);

  /* Read the info line */
  ast->o.file.is_executable = read_exec_package (lex);
  ast->o.file.name = read_name (lex);

  /* After this come the children */
  while (1) {
    struct ast *child = read_child (lex, env);
    if (!child) break;
    ast_add (ast, child);
  }

  return ast;
}

/* Nothing to do */
void free_file (struct ast *ast) { (void) ast; }

void print_file (struct ast *ast, FILE *dest, int indent)
{
  /* (executable "name"
   *   (child...)
   *   (child...))
   */

  int ind;
  size_t i;

  for (ind = 0; ind < indent; ++ind) fputc (' ', dest);
  fprintf (dest, "(%s \"%s\"\n",
           ast->o.file.is_executable ? "executable" : "package",
           ast->o.file.name);
  for (i = 0; i < ast->n_children; ++i) {
    _print_ast (ast->children[i], dest, indent + 2);
    if (i != ast->n_children - 1) fputc ('\n', dest);
  }
  fputc (')', dest);
}
