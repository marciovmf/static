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

 // Create the environment for variable storage
    Environment *env = create_environment();

    /**

     // Sample program
      LET x = 6 + 3 * 3;
      WHILE x > 0
        PRINT x;
        x = x - 1;

     // Expected output
      15
      14
      13
      12
      11
      10
      9
      8
      7
      6
      5
      4
      3
      2
      1
    */

    // === Part 1: x = 6 + 3 * 3; ===

    // Literal 6
    ASTNodeFactor *factor_six = ast_mknode_factor_literal_int(6);
    ASTNodeExpression *expr_six = ast_mknode_expression_factor(factor_six);

    // Literal 3
    ASTNodeFactor *factor_three = ast_mknode_factor_literal_int(3);
    ASTNodeExpression *expr_three = ast_mknode_expression_factor(factor_three);

    // Expression 3 * 3
    ASTNodeExpression *expr_three_times_three = ast_mknode_expression_binary(OP_MULTIPLY, expr_three, expr_three);

    // Expression 6 + (3 * 3)
    ASTNodeExpression *expr_six_plus_three_times_three = ast_mknode_expression_binary(OP_ADD, expr_six, expr_three_times_three);

    // Assignment statement LET x = 6 + 3 * 3;
    ASTNodeStatement *stmt_let_x = ast_mknode_statement_assignment("x", expr_six_plus_three_times_three);

    // === Part 2: WHILE x > 0 ===

    // Variable x
    ASTNodeVariable *variable_x = ast_mknode_variable("x");
    ASTNodeFactor *factor_x = ast_mknode_factor_variable(variable_x);
    ASTNodeExpression *expr_x = ast_mknode_expression_factor(factor_x);

    // Literal 0
    ASTNodeLiteral *literal_zero = ast_mknode_literal_int(0);
    ASTNodeFactor *factor_zero = ast_mknode_factor_literal(literal_zero);
    ASTNodeExpression *expr_zero = ast_mknode_expression_factor(factor_zero);

    // Condition x > 0
    ASTNodeExpression *condition_x_greater_than_zero = ast_mknode_expression_binary(OP_GT, expr_x, expr_zero);

    // === Loop Body of WHILE x > 0 ===

    // Print statement PRINT x;
    ASTNodeStatement *stmt_print_x = ast_mknode_statement_print(expr_x);

    // Decrement statement x = x - 1;
    ASTNodeExpression *expr_one = ast_mknode_expression_factor(ast_mknode_factor_literal(ast_mknode_literal_int(1))); // Literal 1
    ASTNodeExpression *expr_x_minus_one = ast_mknode_expression_binary(OP_SUBTRACT, expr_x, expr_one);
    ASTNodeStatement *stmt_decrement_x = ast_mknode_statement_assignment("x", expr_x_minus_one);

    // Link statements inside the while loop (print -> decrement)
    stmt_print_x->next = stmt_decrement_x;

    // While statement node
    ASTNodeStatement *stmt_while = ast_mknode_statement_while(condition_x_greater_than_zero, stmt_print_x);

    // Link the initial assignment statement to the while loop statement
    stmt_let_x->next = stmt_while;

    // Create program node
    ASTNodeProgram program = { stmt_let_x };

    // Execute the program
    eval_program(&program, env);

    // Clean up environment memory
    free(env);

    return 0;
}

int main_parser(int argc, char **argv)
{
  Lexer lexer;
  char *buffer;
  size_t fileSize;

  if (argc > 1)
  {
    if (read_entire_file_to_memory(argv[1], &buffer, &fileSize, true) != 0)
    {
      return 1; 
    }
  } else
  {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  lexer_init(&lexer, buffer);
  parse_program(&lexer);

  free(buffer);  // Clean up memory after parsing
  return 0;
}

int main(int argc, char **argv)
{
  return main_ast(argc, argv);
}

