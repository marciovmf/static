#ifndef PARSER_H
#define PARSER_H
#include "common.h"  


/* Token */

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

void token_clone(const Token* original, Token* token);

/* Lexer */

typedef struct
{
  char *buffer;        // Buffer containing the file contents
  char current_char;   // Current character
  char next_char;      // Next character
  size_t position;     // Current position in the buffer
  int line;            // Current line number
  int column;          // Current column number
} Lexer;



void  lexer_init(Lexer *lexer, const char *buffer);
Token lexer_get_token(Lexer* lexer);
void  lexer_skip_comment(Lexer* lexer);
void  lexer_skip_whitespace(Lexer *lexer);
void  lexer_advance(Lexer *lexer);
bool  lexer_is_token_type(Token token, TokenType type);
bool  lexer_require_token(Lexer *lexer, TokenType type, Token *out);

/* Parser */
bool parse_program(Lexer *lexer);
bool parse_statement(Lexer *lexer);

#endif // PARSER_H
