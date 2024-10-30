#ifndef AST_H
#define AST_H

#include "common.h"   // for Smallstr

typedef enum ASTNode_e
{
  NODE_LITERAL,
  NODE_VARIABLE,
  NODE_UNARY_OP,
  NODE_BINARY_OP,
  NODE_FUNCTION_CALL,
  NODE_STATEMENT_ASSIGNMENT,
  NODE_STATEMENT_PRINT,
  NODE_STATEMENT_IF,
  NODE_STATEMENT_WHILE,
  NODE_STATEMENT_FOR,
  NODE_STATEMENT_FUNCTION_DECLARATION,
  NODE_PROGRAM,
  NODE_STATEMENT,
  NODE_TERM,
  NODE_FACTOR,
} ASTNodeType;


typedef enum ASTOperator_e
{
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_ASSIGN,
  OP_NOT,
  OP_AND,
  OP_OR,
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  OP_EQ,
  OP_NEQ
} ASTOperator;


typedef enum ASTDataType_e
{
  TYPE_VOID,
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_STRING,
} ASTDataType;


// Forward declare structures
typedef struct ASTNodeLiteral_t       ASTNodeLiteral;
typedef struct ASTNodeVariable_t      ASTNodeVariable;
typedef struct ASTNodeExpression_t    ASTNodeExpression;
typedef struct ASTNodeUnaryOp_t       ASTNodeUnaryOp;
typedef struct ASTNodeBinaryOp_t      ASTNodeBinaryOp;
typedef struct ASTNodeTerm_t          ASTNodeTerm;
typedef struct ASTNodeFactor_t        ASTNodeFactor;
typedef struct ASTNodeAssignment_t    ASTNodeAssignment;
typedef struct ASTNodePrint_t         ASTNodePrint;
typedef struct ASTNodeIf_t            ASTNodeIf;
typedef struct ASTNodeWhile_t         ASTNodeWhile;
typedef struct ASTNodeFor_t           ASTNodeFor;
typedef struct ASTNodeStatement_t     ASTNodeStatement;
typedef struct ASTNodeFunctionDeclaration_t ASTNodeFunctionDeclaration;
typedef struct ASTNodeProgram_t       ASTNodeProgram;

typedef struct ASTNodeLiteral_t
{
  ASTDataType type;
  union
  {
    int     int_value;
    double  float_value;
  } data;
} ASTNodeLiteral;


typedef struct ASTNodeVariable_t
{
  Smallstr  name;
} ASTNodeVariable;


typedef struct ASTNodeUnaryOp_t
{
  ASTOperator         op;
  ASTNodeExpression*  operand;
} ASTNodeUnaryOp;


typedef struct ASTNodeBinaryOp_t
{
  ASTOperator         op;
  ASTNodeExpression*  left;
  ASTNodeExpression*  right;
} ASTNodeBinaryOp;


typedef struct ASTNodeFunctionCall_t
{
  Smallstr            function_name;
  ASTNodeExpression** arguments;
  int                 arg_count;
} ASTNodeFunctionCall;


typedef struct ASTNodeExpression_t
{
  ASTNodeType type;
  union
  {
    ASTNodeLiteral      literal;
    ASTNodeVariable     variable;
    ASTNodeUnaryOp      unary_op;
    ASTNodeBinaryOp     binary_op;
    ASTNodeFunctionCall function_call;
  } data;
} ASTNodeExpression;


struct ASTNodeTerm_t
{
  ASTNodeExpression*  factor;
  ASTNodeTerm*        next;
};


typedef struct ASTNodeFactor_t
{
  ASTNodeType type;
  union
  {
    ASTNodeLiteral      literal;
    ASTNodeVariable     variable;
    ASTNodeExpression*  expression;
  } data;
} ASTNodeFactor;


typedef struct ASTNodeAssignment_t
{
  Smallstr            variable_name; 
  ASTNodeExpression*  expression;
} ASTNodeAssignment;


typedef struct ASTNodePrint_t
{
  ASTNodeExpression *expression;
} ASTNodePrint;


typedef struct ASTNodeIf_t
{
  ASTNodeExpression *condition;
  ASTNodeStatement  *then_branch;
  ASTNodeStatement  *else_branch;
} ASTNodeIf;


typedef struct ASTNodeWhile_t
{
  ASTNodeExpression *condition;
  ASTNodeStatement  *body;
} ASTNodeWhile;


typedef struct ASTNodeFor_t
{
  ASTNodeAssignment *initialization;
  ASTNodeExpression *condition;
  ASTNodeAssignment *increment;
  ASTNodeStatement  *body;
} ASTNodeFor;


typedef struct ASTNodeFunctionDeclaration_t
{
  Smallstr          function_name;
  char**            parameters;
  int               param_count;
  ASTNodeStatement* body;
} ASTNodeFunctionDeclaration;


typedef struct ASTNodeStatement_t
{
  ASTNodeType type;
  union
  {
    ASTNodeAssignment assignment;
    ASTNodePrint      print;
    ASTNodeIf         if_node;
    ASTNodeWhile      while_node;
    ASTNodeFor        for_node;
    ASTNodeFunctionDeclaration function_declaration;
  } data;
  ASTNodeStatement *next; // Next statement in the program
} ASTNodeStatement;


typedef struct ASTNodeProgram_t
{
  ASTNodeStatement *statements; // List of statements
} ASTNodeProgram;

ASTNodeLiteral*     ast_mknode_literal_int(int value);
ASTNodeLiteral*     ast_mknode_literal_float(double value);
ASTNodeVariable*    ast_mknode_variable(const char *name);
ASTNodeExpression*  ast_mknode_expression_unary(ASTOperator op, ASTNodeExpression *operand);
ASTNodeExpression*  ast_mknode_expression_binary(ASTOperator op, ASTNodeExpression *left, ASTNodeExpression *right);
ASTNodeExpression*  ast_mknode_expression_function_call(const char *function_name, ASTNodeExpression **arguments, int argument_count);
ASTNodeTerm*        ast_mknode_term(ASTNodeExpression *factor);
ASTNodeFactor*      ast_mknode_factor_literal(ASTNodeLiteral *literal);
ASTNodeFactor*      ast_mknode_factor_literal_int(int value);
ASTNodeFactor*      ast_mknode_factor_literal_float(float value);
ASTNodeFactor*      ast_mknode_factor_variable(ASTNodeVariable *variable);
ASTNodeFactor*      ast_mknode_factor_expression(ASTNodeExpression *expression);
ASTNodeStatement*   ast_mknode_statement_assignment(const char *variable_name, ASTNodeExpression *expression);
ASTNodeStatement*   ast_mknode_statement_print(ASTNodeExpression *expression);
ASTNodeStatement*   ast_mknode_statement_if(ASTNodeExpression *condition, ASTNodeStatement *then_branch, ASTNodeStatement *else_branch);
ASTNodeStatement*   ast_mknode_statement_while(ASTNodeExpression *condition, ASTNodeStatement *body);
ASTNodeStatement*   ast_mknode_statement_for(ASTNodeAssignment *initialization, ASTNodeExpression *condition, ASTNodeAssignment *increment, ASTNodeStatement *body);
ASTNodeStatement*   ast_mknode_statement_function_declaration(const char *function_name, char **parameters, int parameter_count, ASTNodeStatement *body);
ASTNodeProgram*     ast_mknode_program(ASTNodeStatement *statements);
ASTNodeExpression*  ast_mknode_expression_factor(ASTNodeFactor *factor);
#endif //AST_H

