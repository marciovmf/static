#include "common.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>

#define strdup strdup_safe

//
// Helper functions for ASTExpression nodes
//

ASTExpression* ast_create_expression_binary(ASTExpression* left, ASTOperator op, ASTExpression* right) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_BINARY;
  expr->as.binary_expr.left = left;
  expr->as.binary_expr.op = op;
  expr->as.binary_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_unary(ASTUnaryOperator op, ASTExpression* expression) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_UNARY;
  expr->as.unary_expr.op = op;
  expr->as.unary_expr.expression = expression;
  return expr;
}

ASTExpression* ast_create_expression_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_LOGICAL;
  expr->as.logical_expr.left = left;
  expr->as.logical_expr.op = op;
  expr->as.logical_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_COMPARISON;
  expr->as.comparison_expr.left = left;
  expr->as.comparison_expr.op = op;
  expr->as.comparison_expr.right = right;
  return expr;
}

ASTExpression* ast_create_expression_literal_int(int value) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_LITERAL_INT;
  expr->as.number_literal = (double) value;
  return expr;
}

ASTExpression* ast_create_expression_literal_float(double value) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_LITERAL_FLOAT;
  expr->as.number_literal = value;
  return expr;
}

ASTExpression* ast_create_expression_literal_string(const char* value) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_LITERAL_STRING;
  expr->as.string_literal = strdup(value);
  return expr;
}

ASTExpression* ast_create_expression_lvalue(const char* identifier) 
{
  ASTExpression* expr = (ASTExpression*)malloc(sizeof(ASTExpression));
  expr->type = EXPR_LVALUE;
  expr->as.identifier = strdup(identifier);
  return expr;
}


//
// Helper functions for ASTStatement nodes
//

ASTStatement* ast_create_statement_assignment(const char* identifier, ASTExpression* expression) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_ASSIGNMENT;
  stmt->as.assignment.identifier = strdup(identifier);
  stmt->as.assignment.expression = expression;
  return stmt;
}

ASTStatement* ast_create_statement_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_IF;
  stmt->as.if_stmt.condition = condition;
  stmt->as.if_stmt.if_branch = if_branch;
  stmt->as.if_stmt.else_branch = else_branch;
  return stmt;
}

ASTStatement* ast_create_statement_for(ASTAssignment* init, ASTExpression* condition, ASTAssignment* update, ASTStatement* body) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_FOR;
  stmt->as.for_stmt.init = init;
  stmt->as.for_stmt.condition = condition;
  stmt->as.for_stmt.update = update;
  stmt->as.for_stmt.body = body;
  return stmt;
}

ASTStatement* ast_create_statement_while(ASTExpression* condition, ASTStatement* body) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_WHILE;
  stmt->as.while_stmt.condition = condition;
  stmt->as.while_stmt.body = body;
  return stmt;
}

ASTStatement* ast_create_statement_return(ASTExpression* expression) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_RETURN;
  stmt->as.return_expr = expression;
  return stmt;
}

ASTStatement* ast_create_statement_function_decl(const char* identifier, ASTStatementList* params, ASTBlock* body) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_FUNCTION_DECL;
  stmt->as.function_decl = (ASTFunctionDecl*)malloc(sizeof(ASTFunctionDecl));
  stmt->as.function_decl->identifier = strdup(identifier);
  stmt->as.function_decl->params = params;
  stmt->as.function_decl->body = body;
  return stmt;
}

ASTStatement* ast_create_statement_print(ASTExpression* expression) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_PRINT;
  stmt->as.print_expr = expression;
  return stmt;
}

ASTStatement* ast_create_statement_input(const char* identifier) 
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_INPUT;
  stmt->as.input_expr = ast_create_expression_lvalue(identifier);
  return stmt;
}

ASTStatement* ast_create_statement_break(void)
{
  ASTStatement* stmt = (ASTStatement*)malloc(sizeof(ASTStatement));
  stmt->type = AST_STATEMENT_BREAK;
  return stmt;
}

//
// Helper function to create a new block
//

ASTBlock* ast_create_block(ASTStatementList* statements) 
{
  ASTBlock* block = (ASTBlock*)malloc(sizeof(ASTBlock));
  block->statements = statements;
  return block;
}

ASTStatementList* ast_create_statement_list(ASTStatement* statement, ASTStatementList* next) 
{
  ASTStatementList* stmt_list = (ASTStatementList*)malloc(sizeof(ASTStatementList));
  stmt_list->statement = statement;
  stmt_list->next = next;
  return stmt_list;
}

ASTProgram* ast_create_program(ASTStatementList* statements) 
{
  ASTProgram* program = (ASTProgram*)malloc(sizeof(ASTProgram));
  program->statements = statements;
  return program;
}
