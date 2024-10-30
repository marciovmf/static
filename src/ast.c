#include "common.h"
#include "ast.h"

ASTNodeLiteral* ast_mknode_literal_int(int int_value)
{
  ASTNodeLiteral *node = (ASTNodeLiteral *)malloc(sizeof(ASTNodeLiteral));
  node->type = TYPE_INT;
  node->data.int_value = int_value;
  return node;
}

ASTNodeLiteral* ast_mknode_literal_float(double float_value)
{
  ASTNodeLiteral *node = (ASTNodeLiteral *)malloc(sizeof(ASTNodeLiteral));
  node->type = TYPE_FLOAT;
  node->data.float_value = float_value;
  return node;
}

ASTNodeVariable* ast_mknode_variable(const char *name)
{
  ASTNodeVariable *node = (ASTNodeVariable *)malloc(sizeof(ASTNodeVariable));
  smallstr(&node->name, name);
  return node;
}

ASTNodeExpression* ast_mknode_expression_unary(ASTOperator op, ASTNodeExpression *operand)
{
  ASTNodeExpression *node = (ASTNodeExpression *)malloc(sizeof(ASTNodeExpression));
  node->type = NODE_UNARY_OP;
  node->data.unary_op.op = op;
  node->data.unary_op.operand = operand;
  return node;
}

ASTNodeExpression* ast_mknode_expression_binary(ASTOperator op, ASTNodeExpression *left, ASTNodeExpression *right)
{
  ASTNodeExpression *node = (ASTNodeExpression *)malloc(sizeof(ASTNodeExpression));
  node->type = NODE_BINARY_OP;
  node->data.binary_op.op = op;
  node->data.binary_op.left = left;
  node->data.binary_op.right = right;
  return node;
}

ASTNodeExpression* ast_mknode_expression_function_call(const char *function_name, ASTNodeExpression **arguments, int arg_count)
{
  ASTNodeExpression *node = (ASTNodeExpression *)malloc(sizeof(ASTNodeExpression));
  node->type = NODE_FUNCTION_CALL;
  smallstr(&node->data.function_call.function_name, function_name);
  node->data.function_call.arguments = arguments;
  node->data.function_call.arg_count = arg_count;
  return node;
}

ASTNodeTerm* ast_mknode_term(ASTNodeExpression *factor)
{
  ASTNodeTerm *node = (ASTNodeTerm *)malloc(sizeof(ASTNodeTerm));
  node->factor = factor;
  node->next = NULL;
  return node;
}

ASTNodeFactor* ast_mknode_factor_literal(ASTNodeLiteral *literal)
{
  ASTNodeFactor *node = (ASTNodeFactor *)malloc(sizeof(ASTNodeFactor));
  node->type = NODE_LITERAL;
  node->data.literal = *literal;
  return node;
}

ASTNodeFactor* ast_mknode_factor_literal_int(int literal)
{
  ASTNodeLiteral *l = ast_mknode_literal_int(literal);
  ASTNodeFactor *f = ast_mknode_factor_literal(l);
  return f;
}

ASTNodeFactor* ast_mknode_factor_literal_float(float literal)
{
  ASTNodeLiteral *l = ast_mknode_literal_float(literal);
  ASTNodeFactor *f = ast_mknode_factor_literal(l);
  return f;
}

ASTNodeFactor* ast_mknode_factor_variable(ASTNodeVariable *variable)
{
  ASTNodeFactor *node = (ASTNodeFactor *)malloc(sizeof(ASTNodeFactor));
  node->type = NODE_VARIABLE;
  node->data.variable = *variable;
  return node;
}

ASTNodeFactor* ast_mknode_factor_expression(ASTNodeExpression *expression)
{
  ASTNodeFactor *node = (ASTNodeFactor *)malloc(sizeof(ASTNodeFactor));
  node->type = NODE_FACTOR;
  node->data.expression = expression;
  return node;
}

ASTNodeStatement* ast_mknode_statement_assignment(const char *variable_name, ASTNodeExpression *expression)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_ASSIGNMENT;
  smallstr(&node->data.assignment.variable_name, variable_name);
  node->data.assignment.expression = expression;
  node->next = NULL;
  return node;
}

ASTNodeStatement* ast_mknode_statement_print(ASTNodeExpression *expression)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_PRINT;
  node->data.print.expression = expression;
  node->next = NULL;
  return node;
}

ASTNodeStatement* ast_mknode_statement_if(ASTNodeExpression *condition, ASTNodeStatement *then_branch, ASTNodeStatement *else_branch)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_IF;
  node->data.if_node.condition = condition;
  node->data.if_node.then_branch = then_branch;
  node->data.if_node.else_branch = else_branch;
  node->next = NULL;
  return node;
}

ASTNodeStatement* ast_mknode_statement_while(ASTNodeExpression *condition, ASTNodeStatement *body)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_WHILE;
  node->data.while_node.condition = condition;
  node->data.while_node.body = body;
  node->next = NULL;
  return node;
}

ASTNodeStatement* ast_mknode_statement_for(ASTNodeAssignment *initialization, ASTNodeExpression *condition, ASTNodeAssignment *increment, ASTNodeStatement *body)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_FOR;
  node->data.for_node.initialization = initialization;
  node->data.for_node.condition = condition;
  node->data.for_node.increment = increment;
  node->data.for_node.body = body;
  node->next = NULL;
  return node;
}

ASTNodeStatement* ast_mknode_statement_function_declaration(const char *function_name, char **parameters, int param_count, ASTNodeStatement *body)
{
  ASTNodeStatement *node = (ASTNodeStatement *)malloc(sizeof(ASTNodeStatement));
  node->type = NODE_STATEMENT_FUNCTION_DECLARATION; 
  smallstr(&node->data.function_declaration.function_name, function_name);
  node->data.function_declaration.parameters = parameters;
  node->data.function_declaration.param_count = param_count;
  node->data.function_declaration.body = body;
  node->next = NULL;
  return node;
}

ASTNodeProgram* ast_mknode_program(ASTNodeStatement *statements)
{
  ASTNodeProgram *node = (ASTNodeProgram *)malloc(sizeof(ASTNodeProgram));
  node->statements = statements; // Set the list of statements
  return node;
}

ASTNodeExpression* ast_mknode_expression_factor(ASTNodeFactor *factor)
{
  ASTNodeExpression *node = (ASTNodeExpression *)malloc(sizeof(ASTNodeExpression));
  if (factor->type == NODE_LITERAL)
  {
    node->type = NODE_LITERAL;
    node->data.literal = factor->data.literal;
  }
  else if (factor->type == NODE_VARIABLE)
  {
    node->type = NODE_VARIABLE;
    node->data.variable = factor->data.variable;
  }
  else
  {
    node->type = NODE_UNARY_OP;
    node->data.unary_op.operand = factor->data.expression;
  }
  return node;
}
