#include "parser_utils.h"
#include <iostream>
#include <fstream>

char* readFileToBuffer(const char* fileName, size_t* fileSize)
{
  std::ifstream is(fileName, std::ifstream::binary);
  if(!is)
  {
    logError("Could not open file '%s' for reading\n", fileName);
    return nullptr;
  }

  is.seekg (0, is.end);
  size_t length = is.tellg();
  is.seekg (0, is.beg);

  if(fileSize)
    *fileSize = length;

  char* buffer = new char[length];
  is.read (buffer,length);
  is.close();
  return buffer;
}

bool substrCompare(char* str, char* start, char* end)
{
  const int len = (int)(end - start);
  for(int i=0; i < len; i++)
  {
    if (*str == 0 || *str != tolower(*start))
    {
      return false;
    }
    ++start;
    ++str;
  }
  return true;
}

inline bool isEof(ParseContext& context) 
{
  return context.p >= context.eof; 
}

inline bool isWhiteSpace(char c) 
{
  return c == ' ' || c == '\t'; 
}

char getc(ParseContext& context)
{
  if(isEof(context))
    return END_OF_FILE;

  return *(context.p++);
}

char peek(ParseContext& context)
{
  if(isEof(context))
    return END_OF_FILE;

  return *(context.p);
}

bool isDigit(char c) 
{
  return (c >= '0' && c <= '9');
}

bool isLetter(char c) 
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); 
}

void skipWhiteSpace(ParseContext& context)
{
  while(isWhiteSpace(*context.p))
  {
    getc(context);
  }
}

Token getToken(ParseContext& context)
{
  skipWhiteSpace(context);
  Token token;

  if (isEof(context))
  {
    token.type = Token::Type::TOKEN_EOF;
    return token;
  }

  token.type = Token::Type::TOKEN_UNKNOWN;
  token.start = context.p;
  token.end = token.start;
  char c = getc(context);
  char nextc = peek(context);

  // TOKEN_EXPRESSOIN_START
  if (c == '{' && nextc == '{')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EXPRESSION_START;
    token.end+=2;
    return token;
  }

  // TOKEN_EXPRESSOIN_END
  if (c == '}' && nextc == '}')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EXPRESSION_END;
    token.end+=2;
    return token;
  }

  // TOKEN_ASSIGN
  if (c == '=')
  {
    getc(context);
    token.type = Token::Type::TOKEN_ASSIGN;
    token.end++;
    return token;
  }

  // TOKEN_EOL \n
  if (c == '\n')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EOL;
    token.end++;
    return token;
  }

  if (c == '\r' && nextc == '\n')
  {
    getc(context);
    token.type = Token::Type::TOKEN_EOL;
    token.end+=2;
    return token;
  }

  // TOKEN_PATH
  if (c == '\"')
  {
    ++token.start; // skip starting double quotes
    do{
      if (isEof(context))
      {
        // unexpected EOF while while parsing PATH token;
        token.type = Token::Type::TOKEN_UNKNOWN; 
        return token;
      }
      c = getc(context);
      ++token.end;
    } while (c != '\"');

    token.type = Token::Type::TOKEN_PATH;
    return token;
  }

  if (isLetter(c) || c == '_')
  {
    while(isLetter(c) || isDigit(c) || c == '_' || c == '-' || c == '.')
    {
      ++token.end;
      c = getc(context);
    }
    context.p--;

    if (substrCompare((char*)"for", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_FOR;
    }
    else if (substrCompare((char*)"endfor", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_ENDFOR;
    }
    else if (substrCompare((char*)"in", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_IN;
    }
    else if (substrCompare((char*)"include", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_INCLUDE;
    }
    else if (substrCompare((char*)"all_pages", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_COLLECTION_PAGE;
    }
    else if (substrCompare((char*)"all_posts", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_COLLECTION_POST;
    }
    else if (substrCompare((char*)"orderby_asc", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_ORDERBY_ASC;
    }
    else if (substrCompare((char*)"orderby_desc", token.start, token.end))
    {
      token.type = Token::Type::TOKEN_ORDERBY_DESC;
    }
    else
    {
      token.type = Token::Type::TOKEN_IDENTIFIER;
    }
  }

  return token;
}

