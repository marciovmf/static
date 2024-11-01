#include "ast.h"
#include "common.h"
#include "eval.h"

//
// Runtime expression value
//


inline ExpressionValue runtime_value_create_int(double value)
{
  return (ExpressionValue){ .type = EXPR_LITERAL_INT, .as.number_value = value, .error_code = RUNTIME_SUCCESS };
}

inline ExpressionValue runtime_value_create_float(double value)
{
  return (ExpressionValue){ .type = EXPR_LITERAL_FLOAT, .as.number_value = value, .error_code = RUNTIME_SUCCESS};
}

inline ExpressionValue runtime_value_create_string(char* value)
{
  return (ExpressionValue){ .type = EXPR_LITERAL_STRING, .as.string_value = value, .error_code = RUNTIME_SUCCESS};
}

inline ExpressionValue runtime_value_create_void(void)
{
  return (ExpressionValue){ .type = EXPR_VOID, .error_code = RUNTIME_SUCCESS};
}

inline ExpressionValue runtime_value_create_error(RuntimeError error)
{
  return (ExpressionValue){ .type = EXPR_VOID, .error_code = error};
}

//
// Symbol table functions
//

void symbol_table_init(SymbolTable* table) 
{
  table->count = 0;
}

void symbol_table_set_variable_int(SymbolTable* table, const char* identifier, double value) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->vars[i].identifier.str, identifier) == 0) 
    {
      table->vars[i].value = runtime_value_create_int(value);
      return;
    }
  }
  smallstr(&table->vars[table->count].identifier, identifier);
  table->vars[table->count].value = runtime_value_create_int(value);
  table->count++;
}

void symbol_table_set_variable_float(SymbolTable* table, const char* identifier, double value) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->vars[i].identifier.str, identifier) == 0) 
    {
      table->vars[i].value = runtime_value_create_float(value);
      return;
    }
  }
  smallstr(&table->vars[table->count].identifier, identifier);
  table->vars[table->count].value = runtime_value_create_float(value);
  table->count++;
}

void symbol_table_set_variable_string(SymbolTable* table, const char* identifier, char* value) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->vars[i].identifier.str, identifier) == 0) 
    {
      table->vars[i].value = runtime_value_create_string(value);
      return;
    }
  }
  // If not found, add a new variable
  smallstr(&table->vars[table->count].identifier, identifier);
  table->vars[table->count].value = runtime_value_create_string(value);
  table->count++;
}

Symbol* symbol_table_get_variable(SymbolTable* table, const char* identifier) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->vars[i].identifier.str, identifier) == 0) 
    {
      return &table->vars[i];
    }
  }

  log_warning("Requested uninitialized variable '%s'", identifier);
  return 0;
}


//
// Evaluation fucntions
//

ExpressionValue eval_expression(SymbolTable* table, ASTExpression* expr) 
{
  switch (expr->type) 
  {
    case EXPR_LITERAL_INT:
      return  runtime_value_create_int((int) expr->as.number_literal);
    case EXPR_LITERAL_FLOAT:
      return runtime_value_create_float(expr->as.number_literal);
    case EXPR_LITERAL_STRING:
      return runtime_value_create_string(expr->as.string_literal);
    case EXPR_BINARY:
      ASSERT_BREAK(); // TODO: review  this
      break;
    case EXPR_TERM:
      {
        ExpressionValue left = eval_expression(table, expr->as.binary_expr.left);
        ExpressionValue right = eval_expression(table, expr->as.binary_expr.right);
        ASTExpressionType resultType;

        if (left.type == EXPR_LITERAL_STRING || right.type == EXPR_LITERAL_STRING)
          resultType = EXPR_LITERAL_STRING;
        if (left.type == EXPR_LITERAL_FLOAT || right.type == EXPR_LITERAL_FLOAT)
          resultType = EXPR_LITERAL_FLOAT;
        else
          resultType = EXPR_LITERAL_INT;

        switch (expr->as.binary_expr.op) 
        {
          case OP_ADD:
            {
              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value + right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float((int)left.as.number_value + right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion
                return runtime_value_create_error(RUNTIME_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_SUBTRACT:
            {
              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value - right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value - right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion
                return runtime_value_create_error(RUNTIME_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          default:
            log_and_break("Unknown binary operator for FACTOR");
            return runtime_value_create_void();
            break;
        }
      }
      break;
    case EXPR_FACTOR: 
      {
        ExpressionValue left = eval_expression(table, expr->as.binary_expr.left);
        ExpressionValue right = eval_expression(table, expr->as.binary_expr.right);
        ASTExpressionType resultType;

        if (left.type == EXPR_LITERAL_STRING || right.type == EXPR_LITERAL_STRING)
          resultType = EXPR_LITERAL_STRING;
        if (left.type == EXPR_LITERAL_FLOAT || right.type == EXPR_LITERAL_FLOAT)
          resultType = EXPR_LITERAL_FLOAT;
        else
          resultType = EXPR_LITERAL_INT;
        switch (expr->as.binary_expr.op) 
        {
          case OP_MULTIPLY:
            {
              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return runtime_value_create_error(RUNTIME_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_DIVIDE:
            {

              if (resultType != EXPR_LITERAL_STRING && right.as.number_value == 0.0)
                return runtime_value_create_error(RUNTIME_ERROR_DIVIDE_BY_ZERO);

              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return runtime_value_create_error(RUNTIME_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_MOD:
            {
              if (resultType != EXPR_LITERAL_STRING)
                return runtime_value_create_error(RUNTIME_ERROR_UNSUPPORTED_OPERATION);

              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value * right.as.number_value);
              else
                ASSERT_BREAK();
            }
          default:
            log_and_break("Unknown binary operator for TERM");
            return runtime_value_create_void();
            break;
        }

      } break;
    case EXPR_LVALUE:
      return symbol_table_get_variable(table, expr->as.identifier.str)->value;
    default:
      log_and_break("Unknown expression type");
  }
  return runtime_value_create_void();
}

ExpressionValue eval_statement(SymbolTable* table, ASTStatement* stmt) 
{
  switch (stmt->type) 
  {
    case AST_STATEMENT_ASSIGNMENT: 
      {
        ExpressionValue value = eval_expression(table, stmt->as.assignment.expression);
        ASSERT(value.error_code == RUNTIME_SUCCESS);

        if (value.type == EXPR_LITERAL_INT)
        {
          symbol_table_set_variable_int(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return runtime_value_create_int(value.as.number_value);
        }
        else if (value.type == EXPR_LITERAL_INT)
        {
          symbol_table_set_variable_float(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return runtime_value_create_float(value.as.number_value);
        }
        else if (value.type == EXPR_LITERAL_STRING)
        {
          symbol_table_set_variable_string(table, stmt->as.assignment.identifier.str, value.as.string_value);
          return runtime_value_create_string(value.as.string_value);
        }
        else
        {
          ASSERT_BREAK();
        }
        break;
      }
    case AST_STATEMENT_PRINT: 
      {
        ExpressionValue value = eval_expression(table, stmt->as.print_expr);
        if (value.type == EXPR_LITERAL_INT)
        {
          printf("%d\n", (int) value.as.number_value);
        }
        else if (value.type == EXPR_LITERAL_INT)
        {
          printf("%f\n", value.as.number_value);
        }
        else if (value.type == EXPR_LITERAL_STRING)
        {
          printf("%s\n", value.as.string_value);
        }
        else
        {
          ASSERT_BREAK();
        }

        return runtime_value_create_void();
      }
    case AST_STATEMENT_RETURN: 
      {
        //TODO: Implement AST_STATEMENT_RETURN
        return runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_IF: 
      {
        ExpressionValue condition = condition = eval_expression(table, stmt->as.if_stmt.condition);
        ASSERT(condition.error_code == RUNTIME_SUCCESS && (condition.type == EXPR_LITERAL_FLOAT || condition.type == EXPR_LITERAL_INT));

        if (condition.type != 0) 
        {
          eval_statement(table, stmt->as.if_stmt.if_branch);
        } else if (stmt->as.if_stmt.else_branch) 
        {
          eval_statement(table, stmt->as.if_stmt.else_branch);
        }
        return runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_FOR: 
      {
        //TODO: implement for loops
        break;
      }
    case AST_STATEMENT_WHILE: 
      {
        //TODO: implement while loops
        break;
      }
    case AST_STATEMENT_FUNCTION_DECL: 
      {
        //TODO: implement function declarations
        break;
      }
    case AST_STATEMENT_INPUT: 
      {
        //TODO: implement input statement
        break;
      }
    case AST_STATEMENT_BREAK: 
      {
        //TODO: implement break statement
        break;
      }
    default:
      break;
  }

  return runtime_value_create_void();
}

int eval_program(SymbolTable* table, ASTProgram* program) 
{
  ExpressionValue last_value = {0};

  for (unsigned int i = 0; i < program->body->count; i++)
  {
    ASTStatement* statement = program->body->statements[i];
    last_value = eval_statement(table, statement);
    if (last_value.error_code != RUNTIME_SUCCESS)
      return last_value.error_code;
  }


  if (last_value.type == EXPR_LITERAL_INT)
    return (int) last_value.as.number_value;

  return 0;
}
