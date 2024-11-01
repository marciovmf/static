/**
 * @file ast.h
 * @brief Defines core structures and functions for Abstract Syntax Tree (AST) nodes.
 *
 * Provides data structures and utilities for expressions, statements, 
 * and program constructs in a language's AST.
 *
 * @author marciovmf
 */

#ifndef AST_H
#define AST_H

#include "common.h"   // for Smallstr

typedef struct ASTExpression_t            ASTExpression;
typedef struct ASTStatement_t             ASTStatement;
typedef struct ASTStatementList_t         ASTStatementList;
typedef struct ASTStatementList_t         ASTBlock;
typedef struct ASTFunctionDecl_t          ASTFunctionDecl;
typedef struct ASTAssignment_t            ASTAssignment;
typedef struct ASTIfStatement_t           ASTIfStatement;
typedef struct ASTForStatement_t          ASTForStatement;
typedef struct ASTWhileStatement_t        ASTWhileStatement;
typedef struct ASTReturnStatement_t       ASTReturnStatement;
typedef struct ASTBinaryExpression_t      ASTBinaryExpression;
typedef struct ASTUnaryExpression_t       ASTUnaryExpression;
typedef struct ASTComparisonExpression_t  ASTComparisonExpression;
typedef struct ASTLogicalExpression_t     ASTLogicalExpression;
typedef struct ASTFactor_t                ASTFactor; 
typedef struct ASTTerm_t                  ASTTerm;
typedef struct ASTProgram_t               ASTProgram;

typedef enum ASTOperator_e
{
  OP_ADD,        // Addition
  OP_SUBTRACT,   // Subtraction
  OP_MULTIPLY,   // Multiplication
  OP_DIVIDE,     // Division
  OP_MOD,        // Modulus
  OP_ASSIGN      // Assignment
} ASTOperator;

typedef enum ASTUnaryOperator_e
{
  OP_UNARY_PLUS,   // Unary plus
  OP_UNARY_MINUS,  // Unary minus
  OP_LOGICAL_NOT   // Logical NOT
} ASTUnaryOperator;

typedef enum ASTComparisonOperator_e
{
  OP_LT,    // Less than
  OP_GT,    // Greater than
  OP_LTE,   // Less than or equal to
  OP_GTE,   // Greater than or equal to
  OP_EQ,    // Equal to
  OP_NEQ    // Not equal to
} ASTComparisonOperator;

typedef enum ASTLogicalOperator_e
{
  OP_LOGICAL_AND,  // Logical AND
  OP_LOGICAL_OR    // Logical OR
} ASTLogicalOperator;

typedef enum ASTStatementType_e
{
  AST_STATEMENT_ASSIGNMENT,      // Assignment
  AST_STATEMENT_IF,              // If statement
  AST_STATEMENT_FOR,             // For loop
  AST_STATEMENT_WHILE,           // While loop
  AST_STATEMENT_RETURN,          // Return statement
  AST_STATEMENT_FUNCTION_DECL,   // Function declaration
  AST_STATEMENT_PRINT,           // Print statement
  AST_STATEMENT_INPUT,           // Input statement
  AST_STATEMENT_BREAK            // Break statement
} ASTStatementType;

typedef enum ASTExpressionType_e
{
  EXPR_VOID,               // Void expression
  EXPR_BINARY,             // Binary expression
  EXPR_UNARY,              // Unary expression
  EXPR_COMPARISON,         // Comparison expression
  EXPR_LOGICAL,            // Logical expression
  EXPR_FACTOR,             // Factor for high precedence
  EXPR_TERM,               // Term for low precedence
  EXPR_LITERAL_INT,        // Integer literal
  EXPR_LITERAL_FLOAT,      // Floating-point literal
  EXPR_LITERAL_STRING,     // String literal
  EXPR_LVALUE              // Variable reference
} ASTExpressionType;

struct ASTBinaryExpression_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTOperator op;
};

struct ASTUnaryExpression_t
{
  ASTExpression* expression;
  ASTUnaryOperator op;
};

struct ASTComparisonExpression_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTComparisonOperator op;
};

struct ASTFactor_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTOperator op;
};

struct ASTTerm_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTOperator op;
};

struct ASTLogicalExpression_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTLogicalOperator op;
};

struct ASTExpression_t
{
  ASTExpressionType type;                   // Expression type
  union 
  {
    ASTBinaryExpression     binary_expr;    // Binary expression
    ASTUnaryExpression      unary_expr;     // Unary expression
    ASTComparisonExpression comparison_expr;// Comparison expression
    ASTLogicalExpression    logical_expr;   // Logical expression
    ASTFactor               factor_expr;    // Factor expression
    ASTTerm                 term_expr;      // Term expression
    double                  number_literal; // Numeric literal
    char*                   string_literal; // String literal
    Smallstr                identifier;     // Variable identifier
  } as;
};

struct ASTAssignment_t
{
  Smallstr identifier;
  ASTExpression* expression;
};

struct ASTIfStatement_t
{
  ASTExpression* condition;
  ASTStatement* if_branch;
  ASTStatement* else_branch;
};

struct ASTForStatement_t
{
  ASTAssignment* init;
  ASTExpression* condition;
  ASTAssignment* update;
  ASTStatement* body;
};

struct ASTWhileStatement_t
{
  ASTExpression* condition;
  ASTStatement* body;
};

struct ASTReturnStatement_t
{
  ASTExpression* condition;
};

struct ASTFunctionDecl_t
{
  char*             identifier;
  ASTStatementList* params;
  ASTBlock*         body;
};

struct ASTStatement_t
{
  ASTStatementType type; // Statement type
  union 
  {
    ASTAssignment     assignment;     // Assignment
    ASTIfStatement    if_stmt;        // If statement
    ASTForStatement   for_stmt;       // For loop
    ASTWhileStatement while_stmt;     // While statement
    ASTFunctionDecl   function_decl;  // Function declaration
    ASTExpression*    return_expr;    // Return expression
    ASTExpression*    print_expr;     // Print statement
    ASTExpression*    input_expr;     // Input statement
    ASTBlock* block;                  // Block of statements
  } as;
};

struct ASTStatementList_t 
{
  ASTStatement**  statements; // Array of pointers to ASTStatements
  size_t          count;      // Number of statements in the array
  size_t          capacity;   // Capacity of the array
};


struct ASTProgram_t
{
  ASTBlock* body;
};

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
// Helper functions for creating ASTStatement
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
// Helper functions for Block, ASTSatementList and ASTProgram
//

ASTProgram*       ast_create_program(ASTStatementList* statements);
ASTStatementList* ast_create_statement_list(size_t capacity);
size_t            ast_statement_list_add(ASTStatementList* stmt_list, ASTStatement* statement);


void ast_destroy_expression(ASTExpression* expression);
void ast_destroy_statement(ASTStatement* statement);
void ast_destroy_program(ASTProgram* program);
void ast_destroy_statement_list(ASTStatementList* stmt_list);

#endif // AST_H
