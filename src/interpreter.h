#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

//
// Symbol Table
//

#define MAX_VARS 100

typedef enum RuntimeError_t
{
  RUNTIME_SUCCESS                     = 0,
  RUNTIME_ERROR_DIVIDE_BY_ZERO        = -1024,
  RUNTIME_ERROR_UNSUPPORTED_OPERATION = -1025,
  RUNTIME_ERROR_NOT_IMPLEMENTED       = -1025,
}RuntimeError;

typedef struct ExpressionValue_t
{
  unsigned int error_code;
  ASTExpressionType type;
  union
  {
    double   number_value;
    char* string_value;
  } as;
} ExpressionValue;

typedef struct 
{
  char* identifier;
  ExpressionValue value;
} Symbol;

typedef struct 
{
  Symbol vars[MAX_VARS];
  int count;
} SymbolTable;

void    symbol_table_init(SymbolTable* table);
Symbol* symbol_table_get_variable(SymbolTable* table, const char* identifier);

//
// Evaluation fucntions
//

ExpressionValue eval_expression(SymbolTable* table, ASTExpression* expr);
ExpressionValue eval_statement(SymbolTable* table, ASTStatement* stmt);
int eval_program(SymbolTable* table, ASTProgram* program);

#endif  // INTERPRETER_H
