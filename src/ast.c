#include "common.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>

#define strdup strdup_safe

#define AST_CREATE_NODE(T) (T*)(malloc(sizeof(T)))


void ast_destroy_expression(ASTExpression* expression)
{
  if (expression == NULL)
    return;

  switch (expression->type)
  {
  case EXPR_VOID:
    break;
  case EXPR_UNARY:
    free((void*) expression->as.unary_expr.expression);
  case EXPR_BINARY:
    free((void*) expression->as.binary_expr.left);
    free((void*) expression->as.binary_expr.right);
    break;
  case EXPR_COMPARISON:
    free((void*) expression->as.comparison_expr.left);
    free((void*) expression->as.comparison_expr.right);
    break;
  case EXPR_LOGICAL:
    free((void*) expression->as.logical_expr.left);
    free((void*) expression->as.logical_expr.right);
    break;
  case EXPR_FACTOR:
    free((void*) expression->as.factor_expr.left);
    free((void*) expression->as.factor_expr.right);
  case EXPR_TERM:
    free((void*) expression->as.term_expr.left);
    free((void*) expression->as.term_expr.right);
  case EXPR_LITERAL_STRING:
    free(expression->as.string_literal);
    break;
  case EXPR_LITERAL_INT:
  case EXPR_LITERAL_FLOAT:
  case EXPR_LVALUE:
    break;
  }

  free(expression);
}

void ast_destroy_statement(ASTStatement* statement)
{
  if (statement == NULL)
    return;

  switch(statement->type)
  {
    case AST_STATEMENT_ASSIGNMENT:
      ast_destroy_expression(statement->as.assignment.expression);
      break;
    case AST_STATEMENT_IF:
      ast_destroy_expression(statement->as.if_stmt.condition);
      ast_destroy_statement(statement->as.if_stmt.if_branch);
      ast_destroy_statement(statement->as.if_stmt.else_branch);
      break;
    case AST_STATEMENT_FOR:
      ast_destroy_expression(statement->as.for_stmt.init->expression);
      ast_destroy_expression(statement->as.for_stmt.update->expression);
      ast_destroy_expression(statement->as.for_stmt.condition);
      break;
    case AST_STATEMENT_WHILE:
      ast_destroy_expression(statement->as.while_stmt.condition);
      ast_destroy_statement(statement->as.while_stmt.body);
      break;
    case AST_STATEMENT_RETURN:
      ast_destroy_expression(statement->as.return_expr);
      break;
    case AST_STATEMENT_FUNCTION_DECL:
      ast_destroy_statement_list(statement->as.function_decl.body);
      ast_destroy_statement_list(statement->as.function_decl.params);
      break;
    case AST_STATEMENT_PRINT:
      ast_destroy_expression(statement->as.print_expr);
      break;
    case AST_STATEMENT_INPUT: break;
    case AST_STATEMENT_BREAK: break;
  }

  free(statement);
}

void ast_destroy_program(ASTProgram* program)
{
  for (unsigned int i = 0; i < program->body->count; i++)
  {
    ast_destroy_statement(program->body->statements[i]);
  }

  free(program);
}

//
// Helper functions for ASTExpression nodes
//

ASTExpression* ast_create_expression_binary(ASTExpression* left, ASTOperator op, ASTExpression* right) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);

  if (op == OP_ADD || op == OP_SUBTRACT)
    expr->type = EXPR_TERM;
  else if (op == OP_MULTIPLY || op == OP_DIVIDE || op == OP_MOD)
    expr->type = EXPR_FACTOR;
  else
    ASSERT_BREAK();

  expr->as.binary_expr.left = left;
  expr->as.binary_expr.op = op;
  expr->as.binary_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_unary(ASTUnaryOperator op, ASTExpression* expression) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_UNARY;
  expr->as.unary_expr.op = op;
  expr->as.unary_expr.expression = expression;
  return expr;
}

ASTExpression* ast_create_expression_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LOGICAL;
  expr->as.logical_expr.left = left;
  expr->as.logical_expr.op = op;
  expr->as.logical_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_COMPARISON;
  expr->as.comparison_expr.left = left;
  expr->as.comparison_expr.op = op;
  expr->as.comparison_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_literal_int(int value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_INT;
  expr->as.number_literal = (double) value;
  return expr;
}

ASTExpression* ast_create_expression_literal_float(double value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_FLOAT;
  expr->as.number_literal = value;
  return expr;
}

ASTExpression* ast_create_expression_literal_string(const char* value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_STRING;
  expr->as.string_literal = strdup(value);
  return expr;
}

ASTExpression* ast_create_expression_lvalue(const char* identifier) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LVALUE;
  smallstr(&expr->as.identifier, identifier);
  return expr;
}


//
// Helper functions for ASTStatement nodes
//

ASTStatement* ast_create_statement_assignment(const char* identifier, ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_ASSIGNMENT;
  smallstr(&stmt->as.assignment.identifier, identifier);
  stmt->as.assignment.expression = expression;
  return stmt;
}

ASTStatement* ast_create_statement_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_IF;
  stmt->as.if_stmt.condition = condition;
  stmt->as.if_stmt.if_branch = if_branch;
  stmt->as.if_stmt.else_branch = else_branch;
  return stmt;
}

ASTStatement* ast_create_statement_for(ASTAssignment* init, ASTExpression* condition, ASTAssignment* update, ASTStatement* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FOR;
  stmt->as.for_stmt.init = init;
  stmt->as.for_stmt.condition = condition;
  stmt->as.for_stmt.update = update;
  stmt->as.for_stmt.body = body;
  return stmt;
}

ASTStatement* ast_create_statement_while(ASTExpression* condition, ASTStatement* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_WHILE;
  stmt->as.while_stmt.condition = condition;
  stmt->as.while_stmt.body = body;
  return stmt;
}

ASTStatement* ast_create_statement_return(ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_RETURN;
  stmt->as.return_expr = expression;
  return stmt;
}

ASTStatement* ast_create_statement_function_decl(const char* identifier, ASTStatementList* params, ASTBlock* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FUNCTION_DECL;
  stmt->as.function_decl.identifier = strdup(identifier);
  stmt->as.function_decl.params = params;
  stmt->as.function_decl.body = body;
  return stmt;
}

ASTStatement* ast_create_statement_print(ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_PRINT;
  stmt->as.print_expr = expression;
  return stmt;
}

ASTStatement* ast_create_statement_input(const char* identifier) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_INPUT;
  stmt->as.input_expr = ast_create_expression_lvalue(identifier);
  return stmt;
}

ASTStatement* ast_create_statement_break(void)
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_BREAK;
  return stmt;
}

//
// Helper function to create a new block
//

ASTStatementList* ast_create_statement_list(size_t capacity)
{
  ASSERT(capacity > 0);

  ASTStatementList* stmt_list = AST_CREATE_NODE(ASTStatementList);
  stmt_list->count = 0;
  stmt_list->capacity = capacity;
  stmt_list->statements = (ASTStatement**)(malloc(sizeof(ASTStatement*) * capacity));
  return stmt_list;
}

size_t ast_statement_list_add(ASTStatementList* stmt_list, ASTStatement* statement)
{
  ASSERT(stmt_list != NULL);
  ASSERT(statement != NULL);

  if (stmt_list->count >= stmt_list->capacity)
  {
    stmt_list->capacity *= 2;
    stmt_list->statements = (ASTStatement**)realloc(stmt_list->statements, 
        stmt_list->capacity * sizeof(ASTStatement*));
  }

  stmt_list->statements[stmt_list->count] = statement;
  stmt_list->count++;

  return stmt_list->count;
}

void ast_destroy_statement_list(ASTStatementList* stmt_list)
{
  if (stmt_list == NULL)
    return;

  for (size_t i = 0; i < stmt_list->count; i++)
    free(stmt_list->statements[i]);

  free(stmt_list->statements);
  free(stmt_list);
}

ASTProgram* ast_create_program(ASTStatementList* statements) 
{
  ASTProgram* program = AST_CREATE_NODE(ASTProgram);
  program->body = statements;
  return program;
}
