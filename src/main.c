#include "common.h"
#include "ast.h"
#include "filesystem.h"
#include "config.h"
#include "interpreter.h"
#include "parser.h"

#include <string.h>

#ifdef _WIN32
#define strdup _strdup
#endif


int main_ast(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);

  // Initialize the symbol table
  SymbolTable table;
  symbol_table_init(&table);

  // Create the AST for the expression "a = 2 * 3 + 5"
  ASTExpression* literal_2 = ast_create_expression_literal_int(2);
  ASTExpression* literal_3 = ast_create_expression_literal_int(3);
  ASTExpression* literal_5 = ast_create_expression_literal_int(5);
  ASTExpression* multiplication = ast_create_expression_binary(literal_2, OP_MULTIPLY, literal_3);
  ASTExpression* addition = ast_create_expression_binary(multiplication, OP_ADD, literal_5);

#if 0
  // assignment
  ASTStatement* assignment = ast_create_statement_assignment("a", addition);
  ASTStatementList* stmt_list = ast_create_statement_list(assignment, NULL);
#else 
  // Print
  ASTStatement* print_statement = ast_create_statement_print(addition);
  ASTStatementList* stmt_list = ast_create_statement_list(print_statement, NULL);
  ASTProgram* program = ast_create_program(stmt_list);
#endif

  // Evaluate the program
  int exit_code = eval_program(&table, program);
  printf("Program return code: %d\n", (int) exit_code);

  return 0;
}

int main(int argc, char **argv)
{
  return main_ast(argc, argv);
}

