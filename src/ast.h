#ifndef AST_H
#define AST_H

#include "common.h"   // for Smallstr

typedef struct ASTExpression_t        ASTExpression;
typedef struct ASTStatement_t         ASTStatement;
typedef struct ASTStatementList_t     ASTStatementList;
typedef struct ASTFunctionDecl_t      ASTFunctionDecl;
typedef struct ASTAssignment_t        ASTAssignment;
typedef struct ASTIfStatement_t       ASTIfStatement;
typedef struct ASTForStatement_t      ASTForStatement;
typedef struct ASTWhileStatement_t    ASTWhileStatement;
typedef struct ASTReturnStatement_t   ASTReturnStatement;

//
// Operator Enums
//

typedef enum ASTOperator_e
{
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MOD,
  OP_ASSIGN,
} ASTOperator;

typedef enum ASTUnaryOperator_e
{
  OP_UNARY_PLUS,
  OP_UNARY_MINUS,
  OP_LOGICAL_NOT,  // Logical NOT operator
} ASTUnaryOperator;

typedef enum ASTComparisonOperator_e
{
  OP_LT,
  OP_GT,
  OP_LTE,
  OP_GTE,
  OP_EQ,
  OP_NEQ,
} ASTComparisonOperator;

typedef enum ASTLogicalOperator_e
{
  OP_LOGICAL_AND,  // Logical AND
  OP_LOGICAL_OR    // Logical OR
} ASTLogicalOperator;

typedef enum ASTStatementType_e
{
  AST_STATEMENT_ASSIGNMENT,
  AST_STATEMENT_IF,
  AST_STATEMENT_FOR,
  AST_STATEMENT_WHILE,
  AST_STATEMENT_RETURN,
  AST_STATEMENT_FUNCTION_DECL,
  AST_STATEMENT_PRINT,
  AST_STATEMENT_INPUT,
  AST_STATEMENT_BREAK,
} ASTStatementType;

typedef enum ASTExpressionType_e
{
  EXPR_VOID       = 0,//used in runtime
  EXPR_BINARY,
  EXPR_UNARY,
  EXPR_COMPARISON,
  EXPR_LOGICAL,
  EXPR_FACTOR,
  EXPR_TERM,
  EXPR_LITERAL_INT,
  EXPR_LITERAL_FLOAT,
  EXPR_LITERAL_STRING,
  EXPR_LVALUE,
} ASTExpressionType;

//
// Expression nodes for different precedence levels
//

typedef struct ASTBinaryExpression_t
{
  ASTExpression* left;
  ASTOperator op;
  ASTExpression* right;
} ASTBinaryExpression;

typedef struct ASTUnaryExpression_t
{
  ASTUnaryOperator op;
  ASTExpression* expression;
} ASTUnaryExpression;

typedef struct ASTComparisonExpression_t
{
  ASTExpression* left;
  ASTComparisonOperator op;
  ASTExpression* right;
} ASTComparisonExpression;

typedef struct ASTFactor_t
{
  ASTExpression* left;
  ASTOperator op; // Handles * / % 
  ASTExpression* right;
} ASTFactor;

typedef struct ASTTerm_t
{
  ASTExpression* left;
  ASTOperator op; // Handles + -
  ASTExpression* right;
} ASTTerm;

typedef struct ASTLogicalExpression_t
{
  ASTExpression* left;
  ASTLogicalOperator op; // Handles && and ||
  ASTExpression* right;
} ASTLogicalExpression;

typedef struct ASTExpression_t
{
  ASTExpressionType type;
  union 
  {
    ASTBinaryExpression binary_expr;
    ASTUnaryExpression unary_expr;
    ASTComparisonExpression comparison_expr;
    ASTLogicalExpression logical_expr;
    ASTFactor factor_expr;
    ASTTerm term_expr;
    double number_literal;
    char* string_literal;
    char* identifier;
  } as ;
} ASTExpression;

//
// Statement Nodes
//

typedef struct ASTAssignment_t
{
  char* identifier;
  ASTExpression* expression;
} ASTAssignment;

typedef struct ASTIfStatement_t
{
  ASTExpression* condition;
  ASTStatement* if_branch;
  ASTStatement* else_branch;
} ASTIfStatement;

typedef struct ASTForStatement_t
{
  ASTAssignment* init;
  ASTExpression* condition;
  ASTAssignment* update;
  ASTStatement* body;
} ASTForStatement;

typedef struct ASTWhileStatement_t
{
  ASTExpression* condition;
  ASTStatement* body;
} ASTWhileStatement;

typedef struct ASTReturnStatement_t
{
  ASTExpression* condition;
} ASTReturnStatement;

typedef struct ASTBlock_t
{
  ASTStatementList* statements;
} ASTBlock;

typedef struct ASTStatement_t
{
  ASTStatementType type;
  union 
  {
    ASTAssignment     assignment;
    ASTIfStatement    if_stmt;
    ASTForStatement   for_stmt;
    ASTWhileStatement while_stmt;
    ASTFunctionDecl*  function_decl;
    ASTExpression*    return_expr;
    ASTExpression*    print_expr;
    ASTExpression*    input_expr;
    ASTBlock* block;
  } as;
} ASTStatement_t;

typedef struct ASTStatementList_t 
{
  ASTStatement* statement;
  ASTStatementList* next;
} ASTStatementList;

typedef struct ASTFunctionDecl_t
{
  char* identifier;
  ASTStatementList* params;
  ASTBlock* body;
} ASTFunctionDecl;

typedef struct ASTProgram_t
{
  ASTStatementList* statements;
} ASTProgram;


//
// utility functions
//


//
// Helper functions for ASTExpression nodes
//

ASTExpression* ast_create_expression_binary(ASTExpression* left, ASTOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_unary(ASTUnaryOperator op, ASTExpression* expression);
ASTExpression* ast_create_expression_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_literal_int(int value);
ASTExpression* ast_create_expression_literal_float(double value);
ASTExpression* ast_create_expression_literal_string(const char* value);
ASTExpression* ast_create_expression_lvalue(const char* identifier);

//
// Helper functions for ASTStatement nodes
//

ASTStatement* ast_create_statement_assignment(const char* identifier, ASTExpression* expression);
ASTStatement* ast_create_statement_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch);
ASTStatement* ast_create_statement_for(ASTAssignment* init, ASTExpression* condition, ASTAssignment* update, ASTStatement* body);
ASTStatement* ast_create_statement_while(ASTExpression* condition, ASTStatement* body);
ASTStatement* ast_create_statement_return(ASTExpression* expression);
ASTStatement* ast_create_statement_function_decl(const char* identifier, ASTStatementList* params, ASTBlock* body);
ASTStatement* ast_create_statement_print(ASTExpression* expression);
ASTStatement* ast_create_statement_input(const char* identifier);
ASTStatement* ast_create_statement_break(void);

//
// Helper function to create a new block
//

ASTBlock* ast_create_block(ASTStatementList* statements);
ASTStatementList* ast_create_statement_list(ASTStatement* statement, ASTStatementList* next);
ASTProgram* ast_create_program(ASTStatementList* statements);

#endif // AST_H
