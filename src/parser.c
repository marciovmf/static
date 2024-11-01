/**
 * @file parser.c
 * @brief Defines the lexer and parser for tokenizing and parsing source code.
 *
 * This header contains structures and functions for the lexer and parser, 
 * including token definitions, lexer operations, and parsing functions.
 * 
 * @author marciofmv
 */
#include "common.h"
#include "ast.h"
#include <ctype.h>
#include <string.h>

#define report_error(lexer, message) log_error("Syntax error at line %d, column %d: %s\n",\
    lexer->line, lexer->column, message);

#define report_error_unexpected_token(lexer, token_type) log_error("Sytax error at %d, %d: Unexpected token '%s'\n",\
    lexer->line, lexer->column, token_get_name(token_type))



//
// Lexer
//


#define PARSER_MAX_TOKEN_LENGTH 100

typedef enum
{
  TOKEN_ERROR,              // Represents a tokenizer error
  TOKEN_OP_ASSIGN,          // =
  TOKEN_OP_EQ,              // ==
  TOKEN_OP_NEQ,             // !=
  TOKEN_OP_LT,              // <
  TOKEN_OP_LTE,             // <=
  TOKEN_OP_GT,              // >
  TOKEN_OP_GTE,             // >=
  TOKEN_OPEN_PAREN,         // (
  TOKEN_CLOSE_PAREN,        // )
  TOKEN_OPEN_BRACE,         // {
  TOKEN_CLOSE_BRACE,        // }
  TOKEN_OPEN_BACKET,        // [
  TOKEN_CLOSE_BRACKET,      // ]
  TOKEN_ASTERISK,           // *
  TOKEN_SLASH,              // /
  TOKEN_PERCENT,            // %
  TOKEN_COMMA,              // ,
  TOKEN_DOT,                // .
  TOKEN_EXCLAMATION,        // !
  TOKEN_PLUS,               // +
  TOKEN_MINUS,              // -
  TOKEN_IF,                 // if keyword
  TOKEN_ELSE,               // else keyword
  TOKEN_FOR,                // for keyword
  TOKEN_RETURN,             // return keyword
  TOKEN_INCLUDE,            // include keyword
  TOKEN_FUNCTION,           // function keyword
  TOKEN_IDENTIFIER,         // <variable names>
  TOKEN_LITERAL_INT,        // [0-9]+
  TOKEN_LITERAL_FLOAT,      // [0-9]*"."[0-9]+
  TOKEN_LITERAL_STRING,     // "double quote string!"
  TOKEN_EOF,                // End of file/stream
  TOKEN_COUNT_
} TokenType;


typedef struct
{
  TokenType type;
  char value[PARSER_MAX_TOKEN_LENGTH];
} Token;


typedef struct
{
  char *buffer;        // Buffer containing the file contents
  char current_char;   // Current character
  char next_char;      // Next character
  size_t position;     // Current position in the buffer
  int line;            // Current line number
  int column;          // Current column number
} Lexer;


const char* token_get_name(TokenType token)
{
  static const char *tokenNames[TOKEN_COUNT_] = {
    [TOKEN_OP_ASSIGN]       = "Assignment operator",
    [TOKEN_OP_EQ]           = "Equality operator",
    [TOKEN_OP_NEQ]          = "Inequality operator",
    [TOKEN_OP_LT]           = "Less-than operator",
    [TOKEN_OP_LTE]          = "Less-than-or-equal-to operator",
    [TOKEN_OP_GT]           = "Greater-than operator",
    [TOKEN_OP_GTE]          = "Greater-than-or-equal-to operator",
    [TOKEN_OPEN_PAREN]      = "Open parenthesis",
    [TOKEN_CLOSE_PAREN]     = "Close parenthesis",
    [TOKEN_OPEN_BRACE]      = "Open brace",
    [TOKEN_CLOSE_BRACE]     = "Close brace",
    [TOKEN_OPEN_BACKET]     = "Open bracket",
    [TOKEN_CLOSE_BRACKET]   = "Close bracket",
    [TOKEN_ASTERISK]        = "Multiplication operator",
    [TOKEN_SLASH]           = "Division operator",
    [TOKEN_PERCENT]         = "Modulus operator",
    [TOKEN_COMMA]           = "Comma",
    [TOKEN_DOT]             = "Dot",
    [TOKEN_EXCLAMATION]     = "Logical not operator",
    [TOKEN_PLUS]            = "Sum operator",
    [TOKEN_MINUS]           = "Subtraction operator",
    [TOKEN_IF]              = "if statement",
    [TOKEN_ELSE]            = "else statement",
    [TOKEN_FOR]             = "for statement",
    [TOKEN_RETURN]          = "return satement",
    [TOKEN_INCLUDE]         = "Include directive",
    [TOKEN_IDENTIFIER]      = "Identifier",
    [TOKEN_LITERAL_INT]     = "Integer literal",
    [TOKEN_LITERAL_FLOAT]   = "Floating-point literal",
    [TOKEN_LITERAL_STRING]  = "String literal",
    [TOKEN_EOF]             = "end of file"
  };

  if (token >= 0 && token < TOKEN_COUNT_)
  {
    return tokenNames[token];
  }
  return "Unknown Token";
}


Token lexer_get_next_token(Lexer *lexer);
bool parse_expression(Lexer* lexer);
bool parse_lvalue(Lexer* lexer);


void lexer_init(Lexer *lexer, const char *buffer)
{
  lexer->buffer = (char *)buffer;
  lexer->position = 0;
  lexer->current_char = buffer[lexer->position]; // Start with the first character
  lexer->line = 1;
  lexer->column = 1;
}


void lexer_advance(Lexer *lexer)
{
  if (lexer->current_char == 0)
    return;

  if (lexer->current_char == '\n')
  {
    lexer->line++;
    lexer->column = 1;
  } else
  {
    lexer->column++;
  }
  lexer->position++;
  lexer->current_char = lexer->buffer[lexer->position];
  lexer->next_char = lexer->current_char != 0 ? lexer->buffer[lexer->position + 1] : 0;
}


Token lexer_peek_next_token(Lexer* lexer)
{
  Lexer chekpoint = *lexer;
  Token t = lexer_get_next_token(lexer);
  *lexer = chekpoint;
  return t;
}


void lexer_skip_comment(Lexer *lexer)
{
  while (lexer->current_char != '\n' && lexer->current_char != '\0')
  {
    lexer_advance(lexer);
  }
}


void lexer_skip_whitespace(Lexer *lexer)
{
  while (isspace(lexer->current_char) || lexer->current_char == '#')
  {
    if (lexer->current_char == '#')
    {
      lexer_skip_comment(lexer);  // Skip comment lines
    } else
    {
      lexer_advance(lexer);  // Skip whitespace
    }
  }
}


Token lexer_get_identifier(Lexer *lexer)
{
  Token token;
  int i = 0;

  while (isalpha(lexer->current_char) || lexer->current_char == '_')
  {
    token.value[i++] = lexer->current_char;
    lexer_advance(lexer);
  }
  token.value[i] = '\0';

  if (strcmp(token.value, "if") == 0) token.type = TOKEN_IF;
  else if (strcmp(token.value, "else") == 0) token.type = TOKEN_ELSE;
  else if (strcmp(token.value, "for") == 0) token.type = TOKEN_FOR;
  else if (strcmp(token.value, "include") == 0) token.type = TOKEN_INCLUDE;
  else if (strcmp(token.value, "return") == 0) token.type = TOKEN_RETURN;
  else if (strcmp(token.value, "function") == 0) token.type = TOKEN_FUNCTION;
  //else if (strcmp(token.value, "while") == 0) token.type = TOKEN_WHILE;
  //else if (strcmp(token.value, "endwhile") == 0) token.type = TOKEN_ENDWHILE;
  else token.type = TOKEN_IDENTIFIER;

  return token;
}


Token lexer_get_literal_string(Lexer *lexer)
{
  Token out;

  int i = 0;
  // Expecting a starting quote
  if (lexer->current_char == '"')
  {
    lexer_advance(lexer);  // Move past the opening quote
    while (lexer->current_char != '"' && lexer->current_char != '\0')
    {
      out.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    out.value[i] = '\0';  // Null terminate the string
    if (lexer->current_char == '"')
    {
      lexer_advance(lexer);  // Move past the closing quote
      out.type = TOKEN_LITERAL_STRING;    
    } else
    {
      report_error(lexer, "Unmatched string literal");
      out.type = TOKEN_ERROR;
    }
  } else
  {
    report_error(lexer, "Expected opening quote for string");
    out.type = TOKEN_ERROR;
  }

  return out;
}


bool lexer_skip_token(Lexer* lexer, TokenType expected_type)
{
  Token t = lexer_get_next_token(lexer);
  bool result = (t.type == expected_type);
  if (!result)
    report_error_unexpected_token(lexer, t.type);
  return true;
}


Token lexer_get_next_token(Lexer *lexer)
{
  Token token;
  lexer_skip_whitespace(lexer);

  if (lexer->current_char == '"')
  {
    return lexer_get_literal_string(lexer);
  }

  if (isalpha(lexer->current_char))
  {
    return lexer_get_identifier(lexer);
  } 
  else if (isdigit(lexer->current_char))
  {
    int i = 0;
    while (isdigit(lexer->current_char))
    {
      token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    token.value[i] = '\0';
    token.type = TOKEN_LITERAL_INT;
    return token;
  }
  else if (lexer->current_char == '>' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_GTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_LTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_EQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_NEQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '>')
  {
    token.type = TOKEN_OP_LT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<')
  {
    token.type = TOKEN_OP_LT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=')
  {
    token.type = TOKEN_OP_ASSIGN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '(')
  {
    token.type = TOKEN_OPEN_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ')')
  {
    token.type = TOKEN_CLOSE_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '+')
  {
    token.type = TOKEN_PLUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '-')
  {
    token.type = TOKEN_MINUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '*')
  {
    token.type = TOKEN_ASTERISK;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '/')
  {
    token.type = TOKEN_SLASH;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '%')
  {
    token.type = TOKEN_PERCENT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ',')
  {
    token.type = TOKEN_COMMA;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '.')
  {
    token.type = TOKEN_DOT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!')
  {
    token.type = TOKEN_EXCLAMATION;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '\0')
  {
    token.type = TOKEN_EOF;
    token.value[0] = '\0';
  }
  else
  {
    printf("Unknown character at line %d, column %d: %c\n", lexer->line, lexer->column, lexer->current_char);
    exit(1);
  }

  return token;
}


bool lexer_require_token(Lexer* lexer, TokenType expected_type, Token* out)
{
  *out = lexer_get_next_token(lexer);
  bool result = (out->type == expected_type);
  if (!result)
    report_error_unexpected_token(lexer, out->type);
  return true;
}



//
// Parser
//


/*
* Parses an Argument List
*
* <ArgList> -> [ <Expression> ( "," <Expression> )* ]
* 
*/
bool parse_arg_list(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a Factor
 *
 * <Factor> -> ( int_literal | float_literal | string_literal | <lvalue> | "(" <Expression> ")" )
 */ 
bool parse_factor(Lexer* lexer)
{
  Token look_ahead_token = lexer_peek_next_token(lexer);
  if (look_ahead_token.type == TOKEN_OPEN_PAREN)
  {
    return lexer_skip_token(lexer, TOKEN_OPEN_PAREN)
      && parse_expression(lexer)
      && lexer_skip_token(lexer, TOKEN_CLOSE_PAREN);
  }

  if (look_ahead_token.type == TOKEN_LITERAL_STRING)
  {
    Token literal_string;
    return lexer_require_token(lexer, TOKEN_LITERAL_STRING, &literal_string) &&
      (log_info("%s = '%s'\n", token_get_name(TOKEN_LITERAL_STRING), literal_string.value), true);
  }

  if (look_ahead_token.type == TOKEN_LITERAL_INT)
  {
    Token literal_int;
    return lexer_require_token(lexer, TOKEN_LITERAL_INT, &literal_int);
  }
  
  if (look_ahead_token.type == TOKEN_LITERAL_FLOAT)
  {
    Token literal_float;
    return lexer_require_token(lexer, TOKEN_LITERAL_FLOAT, &literal_float);
  }

  return parse_lvalue(lexer);

  //report_error_unexpected_token(lexer, look_ahead_token.type);
  //return false;
}


/*
 * Parses a UnaryExpression
 *
 * <UnaryExpression> -> [ ( "+" | "-" )] ] <Factor>
 * 
 */ 
bool parse_unary_expression(Lexer* lexer)
{
  Token look_ahead_token = lexer_peek_next_token(lexer);
  if (look_ahead_token.type == TOKEN_PLUS || look_ahead_token.type == TOKEN_MINUS)
  {
    Token sign = lexer_get_next_token(lexer);
    UNUSED(sign);
  }
  return parse_factor(lexer);
};


/*
 * Parses a Term
 *
 * <Term> -> <UnaryExpression> ( ( "*" | "/" | "%" ) <UnaryExpression> )*
 * 
 */ 
bool parse_term(Lexer* lexer)
{
  bool unary_ok = false;
  Token look_ahead_token = {0};
  while(true)
  {
    unary_ok = parse_unary_expression(lexer);
    look_ahead_token = lexer_peek_next_token(lexer);
    if (! unary_ok)
      break;

    if (look_ahead_token.type != TOKEN_ASTERISK && look_ahead_token.type != TOKEN_SLASH &&look_ahead_token.type != TOKEN_PERCENT)
      break;

    Token op = lexer_get_next_token(lexer);
    UNUSED(op);
  }  

  return unary_ok;
};


/*
 * Parses an NumExpression
 *
 * <NumExpression> -> <Term> ( ( "+" | "-" ) <Term> )*
 * 
 */ 
bool parse_num_expression(Lexer* lexer)
{
  bool term_ok = false;
  Token look_ahead_token = {0};
 while(true)
 {
    term_ok = parse_term(lexer);
    look_ahead_token = lexer_peek_next_token(lexer);
    if (! term_ok)
      return false;

    if (look_ahead_token.type != TOKEN_PLUS && look_ahead_token.type != TOKEN_MINUS)
      break;
    
    Token op = lexer_get_next_token(lexer);
    UNUSED(op);
  } 

  return term_ok;
};


/*
 * Parses an Lvalue
 *
 * <lvalue> -> identifier
 */ 
bool parse_lvalue(Lexer* lexer)
{
  Token identifier;
  return lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier);
};


/*
 * Parses an assignment
 *
 * <AssignmentStatement> -> <lvalue> "=" <Expression>
 */
bool parse_assignment_statement(Lexer* lexer)
{
  log_info("Asssignment\n");

  return parse_lvalue(lexer)
    && lexer_skip_token(lexer, TOKEN_OP_ASSIGN)
    && parse_expression(lexer);
};


/*
 * Parses an 'if' statement
 *
 * <ifStatement> -> "if" "(" <Expression> ")" <Statement> [ "else" <Statement> ]
 */
bool parse_if_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a 'for' statement
 *
 * <ForStatement> -> "for" "(" [<AssignmentStatement>] ";" [<expression>] ";" [<AssignmentStatement>] ")" <Statement> 
 */
bool parse_for_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a StatementList
 *
 * <StatementList> -> <Statement> [ <StatementList> ]
 */
bool parse_statement_list(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a ReturnStatement
 *
 * <ReturnStatement> -> "return" [ <Expression> ]
 */
bool parse_return_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a FunctionBody
 *
 * <FunctionBody> -> "(" <ParamList> ")" <Statement>
 */
bool parse_function_body(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses a FunctionDeclStatement
 *
 * <FunctionDeclStatement> -> "function" identifier <FunctionBody>
 */
bool parse_function_declaration_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
};


/*
 * Parses an Expression
 *
 * <Expression> -> <NumExpression> [ ( "<" | ">" | "<=" | ">=" | "==" | "!=" ) <NumExpression> ]
 * 
 */ 
bool parse_expression(Lexer* lexer)
{
  bool expression_ok = false;
  Token look_ahead_token = {0};
  while(true)
  {
    expression_ok = parse_num_expression(lexer);

    if (! expression_ok)
      break;

    look_ahead_token = lexer_peek_next_token(lexer);
    if (look_ahead_token.type != TOKEN_OP_LT && look_ahead_token.type != TOKEN_OP_LTE
        && look_ahead_token.type != TOKEN_OP_GT && look_ahead_token.type != TOKEN_OP_GTE
        && look_ahead_token.type != TOKEN_OP_EQ && look_ahead_token.type != TOKEN_OP_NEQ)
    {
      break;
    }

    Token op = lexer_get_next_token(lexer);
    UNUSED(op);
  }

  return expression_ok;
};


/*
 * Parses a Statement
 *
 * <Statement> -> ( <PrintStatement> | <InputStatement> | <ReturnStatement> | <AssignmentStatement>
 *  <FunctionDeclStatement> | <IfStatement> | <ForStatement> | <WhileStatement> | "{" <StatementList> "}")
 */
bool parse_statement(Lexer *lexer)
{
  Token look_ahead_token = lexer_peek_next_token(lexer);

  switch (look_ahead_token.type)
  {

    case TOKEN_IDENTIFIER:
      return parse_assignment_statement(lexer);

    case TOKEN_OPEN_BRACE:
      return parse_statement(lexer)
        && lexer_skip_token(lexer, TOKEN_CLOSE_BRACE);

    case TOKEN_RETURN:
      return parse_return_statement(lexer);

    case TOKEN_FOR:
      return parse_for_statement(lexer);

    case TOKEN_IF:
      return parse_if_statement(lexer);

    default:
      if (look_ahead_token.type != TOKEN_EOF)
        log_error("Sytax error at %d, %d: Unexpected token '%s' while parsing statement\n",
            lexer->line, lexer->column, token_get_name(look_ahead_token.type));
  }

  return false;
}


/*
* Parses a program
*
* <Program> -> ( <StatementList> )*
*/
bool parse_program(Lexer *lexer)
{
  bool success = true;
  while (true) //lexer->current_char != 0)
  {
    if (!parse_statement(lexer))
    {
      success = false;
      break;
    }
  }
  return success;
}

