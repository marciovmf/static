#include "ast.h"
#include "common.h"
#include "interpreter.h"

Environment* create_environment()
{
  Environment *env = (Environment *) malloc(sizeof(Environment));
  env->entries = NULL;
  env->count = 0;
  return env;
}

// Function to set a variable in the environment
void set_variable_int(Environment *env, const char *name, int value)
{
  for (int i = 0; i < env->count; i++)
  {
    if (strncmp(env->entries[i].name.str, name, env->entries[i].name.length) == 0)
    {
      env->entries[i].value.value.as_int = value;
      env->entries[i].value.type = TYPE_INT;
      return;
    }
  }
  // Add new variable if it does not exist
  env->entries = realloc(env->entries, sizeof(VariableEntry) * (env->count + 1));
  smallstr(&env->entries[env->count].name, name);
  env->entries[env->count].value.value.as_int = value;
  env->entries[env->count].value.type = TYPE_INT;
  env->count++;
}

void set_variable_float(Environment *env, const char *name, double value)
{
  for (int i = 0; i < env->count; i++)
  {
    if (strncmp(env->entries[i].name.str, name, env->entries[i].name.length) == 0)
    {
      env->entries[i].value.value.as_float = value;
      env->entries[i].value.type = TYPE_FLOAT;
      return;
    }
  }
  // Add new variable if it does not exist
  env->entries = realloc(env->entries, sizeof(VariableEntry) * (env->count + 1));
  smallstr(&env->entries[env->count].name, name);
  env->entries[env->count].value.value.as_float = value;
  env->entries[env->count].value.type = TYPE_FLOAT;
  env->count++;
}

// Function to get a variable value from the environment
VariableEntry* get_variable(Environment *env, const char *name)
{
  for (int i = 0; i < env->count; i++)
  {
    if (strncmp(env->entries[i].name.str, name, env->entries[i].name.length) == 0)
    {
      return &env->entries[i];
    }
  }
  log_error("Error: Variable '%s' not found\n", name);
  exit(1);
}

void eval_statement(ASTNodeStatement *stmt, Environment *env)
{
  switch (stmt->type)
  {
    case NODE_STATEMENT_ASSIGNMENT:
      {
        ExpressionValue value = eval_expression(stmt->data.assignment.expression, env);
        if (value.type == TYPE_INT)
          set_variable_int(env, stmt->data.assignment.variable_name.str, value.value.as_int);
        else if (value.type == TYPE_FLOAT)
          set_variable_float(env, stmt->data.assignment.variable_name.str, value.value.as_float);
        else
          ASSERT_BREAK(); // unexpected
        break;
      }
    case NODE_STATEMENT_PRINT:
      {
        ExpressionValue value = eval_expression(stmt->data.print.expression, env);
        if (value.type == TYPE_FLOAT)
          printf("%f\n", value.value.as_float);
        else if (value.type == TYPE_INT)
          printf("%d\n", value.value.as_int);
        else
          ASSERT_BREAK(); // unexpected
        break;
      }
    case NODE_STATEMENT_IF:
      {
        ExpressionValue condition = eval_expression(stmt->data.if_node.condition, env);
        ASSERT(condition.type == TYPE_INT);
        if (condition.value.as_int != 0)
        {
          eval_statement(stmt->data.if_node.then_branch, env);
        }
        else if (stmt->data.if_node.else_branch)
        {
          eval_statement(stmt->data.if_node.else_branch, env);
        }
        break;
      }
    case NODE_STATEMENT_WHILE:
      {
        while (eval_expression(stmt->data.while_node.condition, env).value.as_int != 0)
        {
          ASTNodeStatement* next = stmt->data.while_node.body;
          while (next)
          {
            eval_statement(next, env);
            next = next->next;
          }
        }
        break;
      }
    case NODE_STATEMENT_FOR:
      {
        eval_statement((ASTNodeStatement *)stmt->data.for_node.initialization, env);
        while (eval_expression(stmt->data.for_node.condition, env).value.as_int != 0)
        {
          eval_statement(stmt->data.for_node.body, env);
          eval_statement((ASTNodeStatement *)stmt->data.for_node.increment, env);
        }
        break;
      }
    default:
      fprintf(stderr, "Error: Unsupported statement type %d\n", stmt->type);
      exit(1);
  }
}

static double s_expression_cast_double(ExpressionValue* e)
{
  return (double) (e->type == TYPE_INT ? e->value.as_int : e->value.as_float);
}

static int s_expression_cast_int(ExpressionValue* e)
{
  return (int) (e->type == TYPE_INT ? e->value.as_int : e->value.as_float);
}


// Evaluation for expressions
ExpressionValue eval_expression(ASTNodeExpression *expr, Environment *env)
{
  ExpressionValue value;
  value.type = TYPE_VOID;

  switch (expr->type)
  {
    case NODE_LITERAL:
      if (expr->data.literal.type == TYPE_FLOAT)
      {
        value.type = TYPE_FLOAT;
        value.value.as_float = expr->data.literal.data.float_value;
      }
      else
      {
        value.type = TYPE_INT;
        value.value.as_int = expr->data.literal.data.int_value;
      }
      return value;

    case NODE_VARIABLE:
      VariableEntry* variable = get_variable(env, expr->data.variable.name.str);
      if (variable->value.type == TYPE_FLOAT)
      {
        value.type = TYPE_FLOAT;
        value.value.as_float = variable->value.value.as_float;
      }
      else if (variable->value.type == TYPE_INT)
      {
        value.type = TYPE_INT;
        value.value.as_int = variable->value.value.as_int;
      }
      else
      {
        ASSERT_BREAK();
      }
      return value;

    case NODE_UNARY_OP:
      {
        ExpressionValue operand = eval_expression(expr->data.unary_op.operand, env);
        switch (expr->data.unary_op.op)
        {
          case OP_NOT:
            operand.value.as_float = !operand.value.as_float;
            return operand;
          case OP_SUBTRACT:
            operand.value.as_float = -operand.value.as_float;
            return operand;
          default:
            fprintf(stderr, "Error: Unsupported unary operator %d\n", expr->data.unary_op.op);
            exit(1);
        }
      }

    case NODE_BINARY_OP:
      {
        ExpressionValue left = eval_expression(expr->data.binary_op.left, env);
        ExpressionValue right = eval_expression(expr->data.binary_op.right, env);
        ExpressionValue result = {0};

        ASSERT(left.type == TYPE_INT || left.type == TYPE_FLOAT);
        ASSERT(right.type == TYPE_INT || right.type == TYPE_FLOAT);

        switch (expr->data.binary_op.op)
        {
          case OP_ADD:
            {
              result.type = (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
              double a = s_expression_cast_double(&left);
              double b = s_expression_cast_double(&right);
              double c = a + b;
              (result.type == TYPE_INT) ? (result.value.as_int = (int)c) : (result.value.as_float = c);
              return result;
            }

          case OP_SUBTRACT:
            {
              result.type = (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
              double a = s_expression_cast_double(&left);
              double b = s_expression_cast_double(&right);
              double c = a - b;
              (result.type == TYPE_INT) ? (result.value.as_int = (int)c) : (result.value.as_float = c);
              return result;
            }

          case OP_MULTIPLY: 
            {
              result.type = (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
              double a = s_expression_cast_double(&left);
              double b = s_expression_cast_double(&right);
              double c = a * b;
              (result.type == TYPE_INT) ? (result.value.as_int = (int)c) : (result.value.as_float = c);
              return result;
            }

          case OP_DIVIDE:
            {
              result.type = (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
              double a = s_expression_cast_double(&left);
              double b = s_expression_cast_double(&right);
              double c = a / b;
              (result.type == TYPE_INT) ? (result.value.as_int = (int)c) : (result.value.as_float = c);
              return result;
            }

          case OP_LT:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) < s_expression_cast_int(&right);
              return result;
            }

          case OP_GT:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) > s_expression_cast_int(&right);
              return result;
            }

          case OP_LE:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) <= s_expression_cast_int(&right);
              return result;
            }

          case OP_GE:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) >= s_expression_cast_int(&right);
              return result;
            }

          case OP_EQ:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) == s_expression_cast_int(&right);
              return result;
            }

          case OP_NEQ:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) != s_expression_cast_int(&right);
              return result;
            }

          case OP_AND:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) && s_expression_cast_int(&right);
              return result;
            }

          case OP_OR:
            {
              result.type = TYPE_INT;
              result.value.as_int = s_expression_cast_int(&left) || s_expression_cast_int(&right);
              return result;
            }

          default:
            fprintf(stderr, "Error: Unsupported binary operator %d\n", expr->data.binary_op.op);
            exit(1);
        }
      }

    case NODE_FUNCTION_CALL:
      {
        // Note: This would require a function registry or callback setup
        fprintf(stderr, "Error: Function calls are not supported\n");
        exit(1);
      }

    default:
      fprintf(stderr, "Error: Unsupported expression type %d\n", expr->type);
      exit(1);
  }
}

// Program execution function
void eval_program(ASTNodeProgram *program, Environment *env)
{
  ASTNodeStatement *stmt = program->statements;
  while (stmt)
  {
    eval_statement(stmt, env);
    stmt = stmt->next;
  }
}
