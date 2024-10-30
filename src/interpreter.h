#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

typedef struct RuntimeExpressionValue_t RuntimeExpressionValue;
typedef struct RuntimeVariableEntry_t RuntimeVariableEntry;
typedef struct RuntimeEnviroment_t RuntimeEnviroment;

typedef struct RuntimeExpressionValue_t
{
  ASTDataType type;
  union
  {
    int as_int;
    double as_float;
    char* as_string;
  } value;
} ExpressionValue;

typedef struct RuntimeVariableEntry_t
{
  Smallstr name;
  ExpressionValue value;
} VariableEntry;

typedef struct RuntimeEnviroment_t
{
  VariableEntry *entries;
  int count;
} Environment;


void eval_program(ASTNodeProgram*program, Environment *env);
ExpressionValue eval_expression(ASTNodeExpression *expr, Environment *env);
Environment* create_environment();
void set_variable_int(Environment *env, const char *name, int value);
void set_variable_float(Environment *env, const char *name, double value);
VariableEntry* get_variable(Environment *env, const char *name);


#endif //INTERPRETER_H

