/**
 * @file eval.h
 * @brief Provides structures and functions for interpreting and executing AST nodes.
 *
 * Defines the symbol table for variable management, error handling, and evaluation 
 * functions to execute expressions and statements in the AST.
 * 
 * @author marciovmf
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

//
// Symbol Table
//

#define MAX_VARS 100

/**
 * @enum RuntimeError
 * @brief Defines error codes for runtime evaluation errors.
 */
typedef enum RuntimeError_t
{
  RUNTIME_SUCCESS                     = 0,       // No error.
  RUNTIME_ERROR_DIVIDE_BY_ZERO        = -1024,   // Error for division by zero.
  RUNTIME_ERROR_UNSUPPORTED_OPERATION = -1025,   // Error for an unsupported operation.
  RUNTIME_ERROR_NOT_IMPLEMENTED       = -1026,   // Error for an unimplemented feature.
} RuntimeError;

/**
 * @struct ExpressionValue
 * @brief Represents the evaluated value of an expression, including error handling.
 *
 * Stores the result of an evaluated expression along with error information.
 */
typedef struct ExpressionValue_t
{
  unsigned int error_code;            // Error code, 0 if no error.
  ASTExpressionType type;             // Type of the evaluated expression.
  union
  {
    double number_value;              // Numeric value if the expression is a number
    char* string_value;               // String value if the expression is a string.
  } as;
} ExpressionValue;

/**
 * @struct Symbol
 * @brief Represents a variable and its associated value in the symbol table.
 *
 * Stores the identifier of the variable and its evaluated ExpressionValue.
 */
typedef struct 
{
  Smallstr identifier;                // Variable name.
  ExpressionValue value;              // Current value of the variable.
} Symbol;

/**
 * @struct SymbolTable
 * @brief Holds a collection of symbols (variables) for runtime evaluation.
 *
 * Manages the variables and their values within the current scope.
 */
typedef struct 
{
  Symbol vars[MAX_VARS];              // Array of symbols (variables).
  int count;                          // Number of variables currently stored.
} SymbolTable;

/**
 * @brief Initializes the symbol table.
 * 
 * @param table Pointer to the symbol table to initialize.
 */
void symbol_table_init(SymbolTable* table);

/**
 * @brief Retrieves a variable from the symbol table by its identifier.
 * 
 * @param table Pointer to the symbol table.
 * @param identifier Name of the variable to retrieve.
 * @return Pointer to the Symbol if found, or NULL if not found.
 */
Symbol* symbol_table_get_variable(SymbolTable* table, const char* identifier);

//
// Evaluation
//

/**
 * @brief Evaluates an expression within a given symbol table.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param expr Pointer to the expression AST node to evaluate.
 * @return The evaluated ExpressionValue.
 */
ExpressionValue eval_expression(SymbolTable* table, ASTExpression* expr);

/**
 * @brief Evaluates a statement within a given symbol table.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param stmt Pointer to the statement AST node to evaluate.
 * @return The result of the evaluated statement as an ExpressionValue.
 */
ExpressionValue eval_statement(SymbolTable* table, ASTStatement* stmt);

/**
 * @brief Evaluates the entire program by iterating through the AST.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param program Pointer to the AST program node to evaluate.
 * @return Exit code or runtime status of the program execution.
 */
int eval_program(SymbolTable* table, ASTProgram* program);


#endif  // INTERPRETER_H

