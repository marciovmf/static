#ifndef PARSER_UTILS
#define PARSER_UTILS

#define logError(fmt, ...) printf("ERROR: " fmt, __VA_ARGS__)
#define logInfoFmt(fmt, ...) printf("INFO: " fmt"", __VA_ARGS__)
#define logInfo(msg) printf("INFO: " msg"")
#define END_OF_FILE -1

struct ParseContext
{
  const char* fileName;
  const char* source;
  char* eof;
  char* p;
};

struct Token
{
  enum Type
  {
    TOKEN_ASSIGN            = 0,   // =
    TOKEN_EOL               = 1,   // \n
    TOKEN_EXPRESSION_START  = 2,   // {{
    TOKEN_EXPRESSION_END    = 3,   // }}
    TOKEN_INCLUDE           = 4,   // include
    TOKEN_FOR               = 5,   // for
    TOKEN_ENDFOR            = 6,   // endfor
    TOKEN_IN                = 7,   // in
    TOKEN_IDENTIFIER        = 8,   // similar to C variable name restrictions
    TOKEN_COLLECTION_PAGE   = 9,   // all_pages
    TOKEN_COLLECTION_POST   = 10,  // all_posts
    TOKEN_PATH              = 11,  // Path between double quotes "foo/bar"
    TOKEN_ORDERBY_ASC       = 12,  // orderby_asc reserved word
    TOKEN_ORDERBY_DESC      = 13,  // orderby_desc reserved word
    TOKEN_UNKNOWN           = -1,  // Any unknown token 
    TOKEN_EOF               = -2,  // EOF
  };

  Type type;
  char* start;
  char* end;
};

Token getToken(ParseContext& context);

char* readFileToBuffer(const char* fileName, size_t* fileSize = nullptr);

bool substrCompare(char* str, char* start, char* end);

bool isEof(ParseContext& context);

bool isWhiteSpace(char c); 

char getc(ParseContext& context);

char peek(ParseContext& context);

bool isDigit(char c);

bool isLetter(char c);

void skipWhiteSpace(ParseContext& context);

#endif  // PARSER_UTILS
