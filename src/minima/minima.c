#include "minima.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//
// Lexer
//

typedef struct MiLexer
{
  const char *src;
  size_t      length;
  size_t      pos;
  int         line;
  int         column;
} MiLexer;

static void s_lexer_init(MiLexer *lx, const char *src, size_t len)
{
  lx->src    = src;
  lx->length = len;
  lx->pos    = 0;
  lx->line   = 1;
  lx->column = 1;
}

static bool s_lexer_is_eof(MiLexer *lx)
{
  return lx->pos >= lx->length;
}

static char s_lexer_peek(MiLexer *lx)
{
  if (s_lexer_is_eof(lx))
  {
    return '\0';
  }
  return lx->src[lx->pos];
}

static char s_lexer_advance(MiLexer *lx)
{
  if (s_lexer_is_eof(lx))
  {
    return '\0';
  }

  char c = lx->src[lx->pos];
  lx->pos = lx->pos + 1;

  if (c == '\n')
  {
    lx->line   = lx->line + 1;
    lx->column = 1;
  }
  else
  {
    lx->column = lx->column + 1;
  }

  return c;
}

static void s_lexer_skip_whitespace(MiLexer *lx)
{
  for (;;)
  {
    char c = s_lexer_peek(lx);

    if (c == ' ' || c == '\t' || c == '\r')
    {
      s_lexer_advance(lx);
    }
    else if (c == '#')
    {
      // Commetns to the end of the line; Do not consume the '\n'
      while (!s_lexer_is_eof(lx) && s_lexer_peek(lx) != '\n')
      {
        s_lexer_advance(lx);
      }
    }
    else
    {
      break;
    }
  }
}

static bool s_is_ident_start(char c)
{
  return (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    (c == '_');
}

static bool s_is_ident_part(char c)
{
  return s_is_ident_start(c) || (c >= '0' && c <= '9');
}

static MiToken s_make_token(MiTokenKind kind,
    const char *start,
    size_t      length,
    int         line,
    int         column)
{
  MiToken tok;
  tok.kind = kind;
  tok.lexeme.ptr    = start;
  tok.lexeme.length = length;
  tok.line          = line;
  tok.column        = column;
  return tok;
}

static MiToken s_make_error_token(const char *msg, int line, int column)
{
  MiToken tok;
  tok.kind = MI_TOK_ERROR;
  tok.lexeme.ptr    = msg;
  tok.lexeme.length = strlen(msg);
  tok.line          = line;
  tok.column        = column;
  return tok;
}

static XSlice s_lex_unescape_string(XSlice slice)
{
  char *r = (char*) slice.ptr; // read
  char *w = (char*) slice.ptr; // write
  const char* end = slice.ptr + slice.length;
  size_t len = 0;

  while (r < end )
  {
    if (r[0] == '\\' && r[1] == '\"')
    {
      *w++ = '\"';
      r += 2;
      len++;
    }
    else if (r[0] == '\\' && r[1] == 't')
    {
      *w++ = '\t';
      r += 2;
      len++;
    }
    else if (r[0] == '\\' && r[1] == 'n')
    {
      *w++ = '\n';
      r += 2;
      len++;
    }
    else if (r[0] == '\\' && r[1] == 'r')
    {
      *w++ = '\r';
      r += 2;
      len++;
    }
    else
    {
      *w++ = *r++;
      len++;
    }
  }

  *w = '\0';
  return x_slice_init(slice.ptr, len);
}

static MiToken s_lexer_next(MiLexer *lx)
{
  s_lexer_skip_whitespace(lx);

  if (s_lexer_is_eof(lx))
  {
    return s_make_token(MI_TOK_EOF, lx->src + lx->pos, 0, lx->line, lx->column);
  }

  int   line   = lx->line;
  int   column = lx->column;
  char  c      = s_lexer_advance(lx);
  const char *start = lx->src + (lx->pos - 1);

  // '\n' and ';' produces the same token: MI_TOK_NEWLINE
  if (c == '\n' || c == ';')
  {
    return s_make_token(MI_TOK_NEWLINE, start, 1, line, column);
  }

  // Punctuation
  switch (c)
  {
    case '(' : return s_make_token(MI_TOK_LPAREN,    start, 1, line, column);
    case ')' : return s_make_token(MI_TOK_RPAREN,    start, 1, line, column);
    case '[' : return s_make_token(MI_TOK_LBRACKET,  start, 1, line, column);
    case ']' : return s_make_token(MI_TOK_RBRACKET,  start, 1, line, column);
    case '{' : return s_make_token(MI_TOK_LBRACE,    start, 1, line, column);
    case '}' : return s_make_token(MI_TOK_RBRACE,    start, 1, line, column);
    case ',' : return s_make_token(MI_TOK_COMMA,     start, 1, line, column);
    case ':' :
               {
                 if (s_lexer_peek(lx) == ':')
                 {
                   s_lexer_advance(lx);
                   return s_make_token(MI_TOK_DOUBLE_COLON, start, 2, line, column);
                 }
                 return s_make_token(MI_TOK_COLON, start, 1, line, column);
               }
    case '$' : return s_make_token(MI_TOK_DOLLAR,    start, 1, line, column);
    case '+' : return s_make_token(MI_TOK_PLUS,      start, 1, line, column);
    case '-' : return s_make_token(MI_TOK_MINUS,     start, 1, line, column);
    case '*' : return s_make_token(MI_TOK_STAR,      start, 1, line, column);
    case '/' : return s_make_token(MI_TOK_SLASH,     start, 1, line, column);

    case '=' :
               if (s_lexer_peek(lx) == '=')
               {
                 s_lexer_advance(lx);
                 return s_make_token(MI_TOK_EQEQ, start, 2, line, column);
               }
               break;

    case '!' :
               if (s_lexer_peek(lx) == '=')
               {
                 s_lexer_advance(lx);
                 return s_make_token(MI_TOK_BANGEQ, start, 2, line, column);
               }
               break;

    case '<' :
               if (s_lexer_peek(lx) == '=')
               {
                 s_lexer_advance(lx);
                 return s_make_token(MI_TOK_LTEQ, start, 2, line, column);
               }
               return s_make_token(MI_TOK_LT, start, 1, line, column);

    case '>' :
               if (s_lexer_peek(lx) == '=')
               {
                 s_lexer_advance(lx);
                 return s_make_token(MI_TOK_GTEQ, start, 2, line, column);
               }
               return s_make_token(MI_TOK_GT, start, 1, line, column);

    case '"':
               {
                 const char *open_quote = start;
                 while (!s_lexer_is_eof(lx))
                 {
                   char d = s_lexer_peek(lx);
                   if (d == '"')
                   {
                     break;
                   }
                   if (d == '\\')
                   {
                     s_lexer_advance(lx);
                     char next = s_lexer_peek(lx);
                     if (next == '\"')
                     {
                       s_lexer_advance(lx);
                       continue;
                     }
                   }
                   s_lexer_advance(lx);
                 }

                 if (s_lexer_is_eof(lx))
                 {
                   return s_make_error_token("Unterminated string literal", line, column);
                 }

                 s_lexer_advance(lx); // comsume closing quote

                 const char *after_close = lx->src + lx->pos;
                 const char *inner_start = open_quote + 1;
                 size_t      inner_len   = (size_t)(after_close - inner_start - 1);

                 XSlice expanded = s_lex_unescape_string(x_slice_init(inner_start, inner_len));
                 return s_make_token(MI_TOK_STRING, expanded.ptr, expanded.length, line, column);
               }

    default:
               break;
  }

  // Identifiers and keywords
  if (s_is_ident_start(c))
  {
    while (!s_lexer_is_eof(lx) && s_is_ident_part(s_lexer_peek(lx)))
    {
      s_lexer_advance(lx);
    }

    size_t len = (lx->src + lx->pos) - start;
    XSlice sv  = x_slice_init(start, len);

    if (x_slice_eq_cstr(sv, "true"))
    {
      return s_make_token(MI_TOK_TRUE, start, len, line, column);
    }
    if (x_slice_eq_cstr(sv, "false"))
    {
      return s_make_token(MI_TOK_FALSE, start, len, line, column);
    }
    if (x_slice_eq_cstr(sv, "and"))
    {
      return s_make_token(MI_TOK_AND, start, len, line, column);
    }
    if (x_slice_eq_cstr(sv, "or"))
    {
      return s_make_token(MI_TOK_OR, start, len, line, column);
    }
    if (x_slice_eq_cstr(sv, "not"))
    {
      return s_make_token(MI_TOK_NOT, start, len, line, column);
    }

    return s_make_token(MI_TOK_IDENTIFIER, start, len, line, column);
  }

  // Numbers
  if (c >= '0' && c <= '9')
  {
    bool has_dot = false;

    while (!s_lexer_is_eof(lx))
    {
      char d = s_lexer_peek(lx);
      if (d >= '0' && d <= '9')
      {
        s_lexer_advance(lx);
      }
      else if (d == '.' && !has_dot)
      {
        has_dot = true;
        s_lexer_advance(lx);
      }
      else
      {
        break;
      }
    }

    size_t len = (lx->src + lx->pos) - start;
    return s_make_token(has_dot ? MI_TOK_FLOAT : MI_TOK_INT, start, len, line, column);
  }

  return s_make_error_token("Unexpected character", line, column);
}

//
// Parser
//

typedef struct MiParser
{
  MiLexer  lx;
  MiToken  current;
  MiToken  next;
  bool     has_next;

  bool     had_error;
  int      error_line;
  int      error_column;
  XSlice   error_message;

  XArena  *arena;
} MiParser;

static void s_parser_init(MiParser    *p,
    const char  *src,
    size_t       len,
    XArena      *arena)
{
  memset(p, 0, sizeof(*p));
  s_lexer_init(&p->lx, src, len);
  p->arena = arena;
}

static MiScript   *s_parse_script(MiParser *p, bool stop_at_rbrace);
static MiCommand  *s_parse_command(MiParser *p);
static MiExpr     *s_parse_expr(MiParser *p);
static MiExpr     *s_parse_pair(MiParser *p);
static MiExpr     *s_parse_or(MiParser *p);
static MiExpr     *s_parse_and(MiParser *p);
static MiExpr     *s_parse_equality(MiParser *p);
static MiExpr     *s_parse_comparison(MiParser *p);
static MiExpr     *s_parse_additive(MiParser *p);
static MiExpr     *s_parse_multiplicative(MiParser *p);
static MiExpr     *s_parse_unary(MiParser *p);
static MiExpr     *s_parse_postfix(MiParser *p);
static MiExpr     *s_parse_primary(MiParser *p);

static void s_parser_set_error(MiParser *p, const char *msg, MiToken tok)
{
  if (p->had_error)
  {
    return;
  }

  p->had_error      = true;
  p->error_line     = tok.line;
  p->error_column   = tok.column;
  p->error_message  = x_slice_from_cstr(msg);
}

static MiToken s_parser_advance_raw(MiParser *p)
{
  if (p->has_next)
  {
    p->current  = p->next;
    p->has_next = false;
  }
  else
  {
    p->current = s_lexer_next(&p->lx);
  }
  return p->current;
}

static MiToken s_parser_peek(MiParser *p)
{
  if (!p->has_next)
  {
    p->next     = s_lexer_next(&p->lx);
    p->has_next = true;
  }
  return p->next;
}

static MiToken s_parser_advance(MiParser *p)
{
  return s_parser_advance_raw(p);
}

static bool s_parser_expect(MiParser *p, MiTokenKind kind, const char *msg)
{
  MiToken tok = s_parser_peek(p);
  if (tok.kind != kind)
  {
    s_parser_set_error(p, msg, tok);
    return false;
  }
  s_parser_advance(p);
  return true;
}

//
// AST helpers
//

static inline MiExpr* s_new_expr(MiParser *p, MiExprKind kind, MiToken tok, bool can_fold)
{
  MiExpr *e = (MiExpr*) x_arena_alloc_zero(p->arena, sizeof(MiExpr));
  if (!e)
  {
    s_parser_set_error(p, "Out of memory", tok);
    return NULL;
  }
  e->kind  = kind;
  e->token = tok;
  e->can_fold = can_fold;
  return e;
}

static MiExprList* s_expr_list_append(MiParser *p, MiExprList *head, MiExpr *expr)
{
  MiExprList *node = (MiExprList*) x_arena_alloc_zero(p->arena, sizeof(MiExprList));
  if (!node)
  {
    s_parser_set_error(p, "Out of memory", expr ? expr->token : p->current);
    return head;
  }
  node->expr = expr;
  node->next = NULL;

  if (!head)
  {
    return node;
  }

  MiExprList *it = head;
  while (it->next)
  {
    it = it->next;
  }
  it->next = node;
  return head;
}

static MiCommand* s_new_command(MiParser *p, MiExpr *head, int argc, MiExprList *args, MiToken head_tok)
{
  MiCommand *cmd = (MiCommand*) x_arena_alloc_zero(p->arena, sizeof(MiCommand));
  if (!cmd)
  {
    s_parser_set_error(p, "Out of memory", head_tok);
    return NULL;
  }
  cmd->head = head;
  cmd->argc = argc;
  cmd->args = args;
  return cmd;
}

static MiCommandList* s_command_list_append(MiParser *p,
    MiCommandList *head,
    MiCommand     *cmd)
{
  MiCommandList *node = (MiCommandList*) x_arena_alloc_zero(p->arena, sizeof(MiCommandList));
  if (!node)
  {
    MiToken tok = cmd && cmd->head ? cmd->head->token : p->current;
    s_parser_set_error(p, "Out of memory", tok);
    return head;
  }
  node->command = cmd;
  node->next    = NULL;

  if (!head)
  {
    return node;
  }

  MiCommandList *it = head;
  while (it->next)
  {
    it = it->next;
  }
  it->next = node;
  return head;
}

static MiScript* s_new_script(MiParser *p)
{
  MiScript *scr = (MiScript*) x_arena_alloc_zero(p->arena, sizeof(MiScript));
  if (!scr)
  {
    MiToken tok = s_parser_peek(p);
    s_parser_set_error(p, "Out of memory", tok);
    return NULL;
  }
  return scr;
}

//
// Script and Command parsing
//

static void s_parser_sync_newline(MiParser *p)
{
  while (s_parser_peek(p).kind == MI_TOK_NEWLINE)
  {
    s_parser_advance(p);
  }
}

/**
 * Ignore \n right after a '{','(','[' or ','
 */
static void s_skip_newlines_for_group(MiParser *p)
{
  while (s_parser_peek(p).kind == MI_TOK_NEWLINE)
  {
    s_parser_advance(p);
  }
}

/**
 * script ::= (command_line (NEWLINE)*)* EOF
 */ 
static MiScript* s_parse_script(MiParser *p, bool stop_at_rbrace)
{
  MiScript      *scr  = s_new_script(p);
  MiCommandList *list = NULL;
  size_t         count = 0;

  s_parser_sync_newline(p);

  while (!p->had_error)
  {
    MiToken tok = s_parser_peek(p);

    // End of file
    if (tok.kind == MI_TOK_EOF)
    {
      break;
    }

    // Inside { }, we stop BEFORE consuming the closing '}'
    if (stop_at_rbrace && tok.kind == MI_TOK_RBRACE)
    {
      break;
    }

    // just skip end of line
    if (tok.kind == MI_TOK_NEWLINE)
    {
      s_parser_advance(p);
      continue;
    }

    // here we expect a command
    MiCommand *cmd = s_parse_command(p);
    if (!cmd)
    {
      if (p->had_error)
      {
        printf("Parsing error %d,%d - %.*s\n", p->error_line, p->error_column, (int) p->error_message.length, p->error_message.ptr);
        break;
      }
      else
      {
        break;
      }
    }

    list  = s_command_list_append(p, list, cmd);
    count = count + 1;

    // Consume command separators (newline / ';')
    while (s_parser_peek(p).kind == MI_TOK_NEWLINE)
    {
      s_parser_advance(p);
    }
  }

  scr->first         = list;
  scr->command_count = count;
  return scr;
}

/**
 * command ::= head_expr "::" arg_expr*
 */
static MiCommand* s_parse_command(MiParser *p)
{
  // Parses the full line as ONE expression
  MiExpr *expr = s_parse_expr(p);
  if (!expr)
  {
    return NULL;
  }

  // On script level expression MUST be a command lie name_expression :: args
  if (expr->kind != MI_EXPR_COMMAND)
  {
    s_parser_set_error(p,
        "Expected 'head_expr :: arg_expr*' command",
        expr->token);
    return NULL;
  }

  // Convert MI_EXPR_COMMAND into MiCommand
  MiCommand *cmd = s_new_command(p,
      expr->as.command.head,
      expr->as.command.argc,
      expr->as.command.args,
      expr->token);
  return cmd;
}

//
// Expression parsing / Precedence
//

static bool s_tokens_adjacent(const MiToken *a, const MiToken *b)
{
  const char *end_a = a->lexeme.ptr + a->lexeme.length;
  return end_a == b->lexeme.ptr;
}

/**
 * expr ::= pair_expr
 */
static MiExpr* s_parse_expr(MiParser *p)
{
  return s_parse_pair(p);
}

/**
 * pair_expr ::= or_expr ( ":" expr )?
 */
static MiExpr* s_parse_pair(MiParser *p)
{
  MiExpr *left = s_parse_or(p);
  if (!left)
  {
    return NULL;
  }

  if (s_parser_peek(p).kind == MI_TOK_COLON)
  {
    MiToken colon = s_parser_advance(p);
    MiExpr *right = s_parse_expr(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *pair = s_new_expr(p, MI_EXPR_PAIR, colon, false);
    if (!pair)
    {
      return NULL;
    }
    pair->as.pair.key   = left;
    pair->as.pair.value = right;
    return pair;
  }

  return left;
}

/**
 * postfix_expr ::= primary_expr ("[" expr "]")* ("::" arg_expr*)?
 */
static MiExpr* s_parse_postfix(MiParser *p)
{
  MiExpr *expr = s_parse_primary(p);
  if (!expr)
  {
    return NULL;
  }

  // Expressing indexing like expr[index] only if no spaces between expr and '['
  for (;;)
  {
    MiToken next = s_parser_peek(p);

    if (next.kind == MI_TOK_LBRACKET)
    {
      MiToken prev = p->current;

      if (!s_tokens_adjacent(&prev, &next))
      {
        // If there is space between expr and [] this isn't indexing. Stop!
        break;
      }

      // indexing: expr[index]
      MiToken lbrack = s_parser_advance(p);

      // Empty lines are allowed right after '['
      s_skip_newlines_for_group(p);

      MiExpr *index = s_parse_expr(p);
      if (!index)
      {
        return NULL;
      }

      // Empty lines are allowed before ']'
      s_skip_newlines_for_group(p);

      if (!s_parser_expect(p, MI_TOK_RBRACKET, "Expected ']' after index expression"))
      {
        return NULL;
      }

      MiExpr *idx = 
        s_new_expr(p, MI_EXPR_INDEX, lbrack,
            (index->can_fold &&  expr->can_fold));

      if (!idx)
      {
        return NULL;
      }


      idx->as.index.target = expr;
      idx->as.index.index  = index;
      expr = idx;

      continue;
    }

    break;
  }

  // Command call from inside the expression: head_expr :: arg_expr*
  if (s_parser_peek(p).kind == MI_TOK_DOUBLE_COLON)
  {
    unsigned int argc = 0;
    MiToken colon_tok = s_parser_advance(p);
    MiExprList *args  = NULL;

    bool can_fold = true;
    for (;;)
    {
      MiTokenKind k = s_parser_peek(p).kind;

      // Here we stop collecting arguments for the command:
      // OBS: we do NOT consume '}' here; That's s_parse_script responsibility
      if (k == MI_TOK_EOF ||
          k == MI_TOK_NEWLINE ||
          k == MI_TOK_RPAREN ||
          k == MI_TOK_RBRACE ||
          k == MI_TOK_RBRACKET)
      {
        break;
      }

      // We also stop collecting arguments for the command if the current token
      // does not start a new expression
      if (k != MI_TOK_INT &&
          k != MI_TOK_FLOAT &&
          k != MI_TOK_STRING &&
          k != MI_TOK_TRUE &&
          k != MI_TOK_FALSE &&
          k != MI_TOK_DOLLAR &&
          k != MI_TOK_IDENTIFIER &&
          k != MI_TOK_LPAREN &&
          k != MI_TOK_LBRACKET &&
          k != MI_TOK_LBRACE &&
          k != MI_TOK_MINUS &&
          k != MI_TOK_NOT)
      {
        break;
      }

      // Each argument is a complete expression
      MiExpr *arg = s_parse_expr(p);
      if (!arg)
      {
        return NULL;
      }

      can_fold &= arg->can_fold;
      argc++;
      args = s_expr_list_append(p, args, arg);
    }


    MiExpr *cmd = s_new_expr(p, MI_EXPR_COMMAND, colon_tok, can_fold);

    if (!cmd)
    {
      return NULL;
    }
    cmd->as.command.head = expr;
    cmd->as.command.args = args;
    cmd->as.command.argc = argc;
    expr = cmd;
  }

  return expr;
}

static MiExpr* s_parse_or(MiParser *p)
{
  MiExpr *left = s_parse_and(p);
  if (!left)
  {
    return NULL;
  }

  while (s_parser_peek(p).kind == MI_TOK_OR)
  {
    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_and(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold));

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_and(MiParser *p)
{
  MiExpr *left = s_parse_equality(p);
  if (!left)
  {
    return NULL;
  }

  while (s_parser_peek(p).kind == MI_TOK_AND)
  {
    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_equality(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold));

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_equality(MiParser *p)
{
  MiExpr *left = s_parse_comparison(p);
  if (!left)
  {
    return NULL;
  }

  for (;;)
  {
    MiTokenKind kind = s_parser_peek(p).kind;
    if (kind != MI_TOK_EQEQ && kind != MI_TOK_BANGEQ)
    {
      break;
    }
    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_comparison(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold));

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_comparison(MiParser *p)
{
  MiExpr *left = s_parse_additive(p);
  if (!left)
  {
    return NULL;
  }

  for (;;)
  {
    MiTokenKind kind = s_parser_peek(p).kind;
    if (kind != MI_TOK_LT &&
        kind != MI_TOK_GT &&
        kind != MI_TOK_LTEQ &&
        kind != MI_TOK_GTEQ)
    {
      break;
    }

    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_additive(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold));

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_additive(MiParser *p)
{
  MiExpr *left = s_parse_multiplicative(p);
  if (!left)
  {
    return NULL;
  }

  for (;;)
  {
    MiTokenKind kind = s_parser_peek(p).kind;
    if (kind != MI_TOK_PLUS && kind != MI_TOK_MINUS)
    {
      break;
    }

    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_multiplicative(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold));

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_multiplicative(MiParser *p)
{
  MiExpr *left = s_parse_unary(p);
  if (!left)
  {
    return NULL;
  }

  for (;;)
  {
    MiTokenKind kind = s_parser_peek(p).kind;
    if (kind != MI_TOK_STAR && kind != MI_TOK_SLASH)
    {
      break;
    }

    MiToken op_tok = s_parser_advance(p);
    MiExpr *right  = s_parse_unary(p);
    if (!right)
    {
      return NULL;
    }

    MiExpr *bin = s_new_expr(p, MI_EXPR_BINARY, op_tok,
        (left->can_fold && right->can_fold) );

    if (!bin)
    {
      return NULL;
    }
    bin->as.binary.op    = op_tok.kind;
    bin->as.binary.left  = left;
    bin->as.binary.right = right;
    left = bin;
  }

  return left;
}

static MiExpr* s_parse_unary(MiParser *p)
{
  MiTokenKind kind = s_parser_peek(p).kind;

  if (kind == MI_TOK_MINUS || kind == MI_TOK_NOT)
  {
    MiToken op_tok = s_parser_advance(p);
    MiExpr *expr   = s_parse_unary(p);
    if (!expr)
    {
      return NULL;
    }

    MiExpr *u = s_new_expr(p, MI_EXPR_UNARY, op_tok, expr->can_fold);

    if (!u)
    {
      return NULL;
    }
    u->as.unary.op   = op_tok.kind;
    u->as.unary.expr = expr;
    return u;
  }

  return s_parse_postfix(p);
}

static MiExpr* s_parse_list_or_dict(MiParser *p)
{
  // '[' was already consumed
  MiToken lbrack = p->current;

  MiExprList *items = NULL;

  // right after '[' empty lines are allowed
  s_skip_newlines_for_group(p);

  if (s_parser_peek(p).kind == MI_TOK_RBRACKET)
  {
    s_parser_advance(p); // skip ']' empty list
    MiExpr *list = s_new_expr(p, MI_EXPR_LIST, lbrack, true);
    if (!list)
    {
      return NULL;
    }
    list->as.list.items = NULL;
    return list;
  }

  bool all_pairs = true;
  bool can_fold = true;

  for (;;)
  {
    MiExpr *first = s_parse_expr(p);
    if (!first)
    {
      return NULL;
    }

    MiExpr *elem_expr = NULL;

    // try matching "key : value"
    if (s_parser_peek(p).kind == MI_TOK_COLON)
    {
      MiToken colon = s_parser_advance(p);
      MiExpr *second = s_parse_expr(p);
      if (!second)
      {
        return NULL;
      }

      MiExpr *pair = s_new_expr(p, MI_EXPR_PAIR, colon, false);
      if (!pair)
      {
        return NULL;
      }
      pair->as.pair.key   = first;
      pair->as.pair.value = second;
      elem_expr = pair;
      can_fold &= (first->can_fold & second->can_fold);

    }
    else
    {
      elem_expr = first;
      all_pairs = false;
      can_fold &= first->can_fold;
    }

    items = s_expr_list_append(p, items, elem_expr);

    if (!items)
    {
      return NULL;
    }

    // Empty lines are allowed right before ',' or ']'
    s_skip_newlines_for_group(p);

    if (s_parser_peek(p).kind == MI_TOK_COMMA)
    {
      s_parser_advance(p); // ','

      // allow empty lines after ',' 
      s_skip_newlines_for_group(p);

      if (s_parser_peek(p).kind == MI_TOK_RBRACKET)
      {
        // optional dangling ','
        break;
      }
      continue;
    }
    else
    {
      break;
    }
  }

  // right before ']' empty lines are allowed
  s_skip_newlines_for_group(p);

  if (!s_parser_expect(p, MI_TOK_RBRACKET, "Expected ']' to close list/dict literal"))
  {
    return NULL;
  }

  MiExpr *expr = s_new_expr(p, all_pairs ? MI_EXPR_DICT : MI_EXPR_LIST, lbrack, false);
  if (!expr)
  {
    return NULL;
  }

  if (all_pairs)
  {
    expr->as.dict.items = items;
  }
  else
  {
    expr->as.list.items = items;
  }

  expr->can_fold = can_fold;

  return expr;
}

static MiExpr* s_parse_block_literal(MiParser *p)
{
  // '{' was already consumed
  MiToken lbrace = p->current;

  MiScript *script = s_parse_script(p, true);
  if (!script)
  {
    return NULL;
  }

  if (!s_parser_expect(p, MI_TOK_RBRACE, "Expected '}' at end of block"))
  {
    return NULL;
  }

  MiExpr *expr = s_new_expr(p, MI_EXPR_BLOCK, lbrace, false);
  if (!expr)
  {
    return NULL;
  }
  expr->as.block.script = script;
  return expr;
}

static MiExpr* s_parse_primary(MiParser *p)
{
  MiToken tok = s_parser_peek(p);

  switch (tok.kind)
  {
    case MI_TOK_INT:
      {
        s_parser_advance(p);
        MiExpr *e = s_new_expr(p, MI_EXPR_INT_LITERAL, tok, true);
        if (!e)
        {
          return NULL;
        }
        char buf[64];
        size_t n = tok.lexeme.length;
        if (n >= sizeof(buf))
        {
          n = sizeof(buf) - 1;
        }
        memcpy(buf, tok.lexeme.ptr, n);
        buf[n] = '\0';
        e->as.int_lit.value = (int64_t) strtoll(buf, NULL, 10);
        return e;
      }

    case MI_TOK_FLOAT:
      {
        s_parser_advance(p);
        MiExpr *e = s_new_expr(p, MI_EXPR_FLOAT_LITERAL, tok, true);
        if (!e)
        {
          return NULL;
        }
        char buf[64];
        size_t n = tok.lexeme.length;
        if (n >= sizeof(buf))
        {
          n = sizeof(buf) - 1;
        }
        memcpy(buf, tok.lexeme.ptr, n);
        buf[n] = '\0';
        e->as.float_lit.value = strtod(buf, NULL);
        return e;
      }

    case MI_TOK_STRING:
      {
        s_parser_advance(p);
        MiExpr *e = s_new_expr(p, MI_EXPR_STRING_LITERAL, tok, true);
        if (!e)
        {
          return NULL;
        }
        e->as.string_lit.value = tok.lexeme;
        return e;
      }

    case MI_TOK_TRUE:
    case MI_TOK_FALSE:
      {
        s_parser_advance(p);
        MiExpr *e = s_new_expr(p, MI_EXPR_BOOL_LITERAL, tok, true);
        if (!e)
        {
          return NULL;
        }
        e->as.bool_lit.value = (tok.kind == MI_TOK_TRUE);
        return e;
      }

    case MI_TOK_LPAREN:
      {

        // '(' was already consumed
        s_parser_advance(p);

        // Empty lines are allowed right after '('
        s_skip_newlines_for_group(p);

        if (s_parser_peek(p).kind == MI_TOK_RPAREN)
        {
          // literal void: () or (⏎⏎)
          MiToken rp = s_parser_advance(p);
          MiExpr *e  = s_new_expr(p, MI_EXPR_VOID_LITERAL, rp, true);
          if (!e)
          {
            return NULL;
          }
          return e;
        }

        MiExpr *inner = s_parse_expr(p);
        if (!inner)
        {
          return NULL;
        }

        // Empty lines are allowed right before ')'
        s_skip_newlines_for_group(p);

        if (!s_parser_expect(p, MI_TOK_RPAREN, "Expected ')' after expression"))
        {
          return NULL;
        }
        return inner;
      }

    case MI_TOK_LBRACKET:
      {
        s_parser_advance(p); // '['
        return s_parse_list_or_dict(p);
      }

    case MI_TOK_LBRACE:
      {
        s_parser_advance(p); // '{'
        return s_parse_block_literal(p);
      }

    case MI_TOK_DOLLAR:
      {
        s_parser_advance(p); // '$'
        MiToken name_tok = s_parser_peek(p);
        if (name_tok.kind != MI_TOK_IDENTIFIER)
        {
          s_parser_set_error(p, "Expected identifier after '$'", name_tok);
          return NULL;
        }
        s_parser_advance(p);

        MiExpr *e = s_new_expr(p, MI_EXPR_VAR, name_tok, false);
        if (!e)
        {
          return NULL;
        }
        e->as.var.name = name_tok.lexeme;
        return e;
      }

    case MI_TOK_IDENTIFIER:
      {
        s_parser_advance(p);
        MiExpr *e = s_new_expr(p, MI_EXPR_STRING_LITERAL, tok, true);
        if (!e)
        {
          return NULL;
        }
        e->as.string_lit.value = tok.lexeme;
        return e;
      }

    default:
      s_parser_set_error(p, "Unexpected token in expression", tok);
      return NULL;
  }
}

//
// Public API
//


#if defined(_DEBUG) || defined(DEBUG)

//
// AST Debugging
//

void mi_ast_debug_print_script_indent(const MiScript *script, int indent);

static void s_print_indent(int indent)
{
  int i;
  for (i = 0; i < indent; ++i)
  {
    fputs("  ", stdout);
  }
}

static void s_print_slice(XSlice sv)
{
  fwrite(sv.ptr, 1, sv.length, stdout);
}

void mi_ast_debug_print_expr(const MiExpr *expr, int indent)
{
  if (!expr)
  {
    s_print_indent(indent);
    printf("<null-expr>\n");
    return;
  }

  const char* can_fold = expr->can_fold ? "[FOLD]" : "";

  switch (expr->kind)
  {
    case MI_EXPR_INT_LITERAL:
      s_print_indent(indent);
      printf("%s INT(%lld)\n", can_fold, (long long) expr->as.int_lit.value);
      break;

    case MI_EXPR_FLOAT_LITERAL:
      s_print_indent(indent);
      printf("%s FLOAT(%f)\n", can_fold, expr->as.float_lit.value);
      break;

    case MI_EXPR_STRING_LITERAL:
      s_print_indent(indent);
      printf("%s STRING(\"", can_fold);
      s_print_slice(expr->as.string_lit.value);
      printf("\")\n");
      break;

    case MI_EXPR_BOOL_LITERAL:
      s_print_indent(indent);
      printf("%s BOOL(%s)\n", can_fold, expr->as.bool_lit.value ? "true" : "false");
      break;

    case MI_EXPR_VOID_LITERAL:
      s_print_indent(indent);
      printf("%s VOID()\n", can_fold);
      break;

    case MI_EXPR_VAR:
      s_print_indent(indent);
      printf("%s VAR(", can_fold);
      s_print_slice(expr->as.var.name);
      printf(")\n");
      break;

    case MI_EXPR_INDEX:
      s_print_indent(indent);
      printf("%s INDEX:\n", can_fold);
      s_print_indent(indent + 1);
      printf("TARGET:\n");
      mi_ast_debug_print_expr(expr->as.index.target, indent + 2);
      s_print_indent(indent + 1);
      printf("INDEX:\n");
      mi_ast_debug_print_expr(expr->as.index.index, indent + 2);
      break;

    case MI_EXPR_UNARY:
      s_print_indent(indent);
      printf("%s UNARY(", can_fold);
      switch (expr->as.unary.op)
      {
        case MI_TOK_MINUS: printf("-"); break;
        case MI_TOK_NOT:   printf("not"); break;
        default:           printf("op?"); break;
      }
      printf(")\n");
      mi_ast_debug_print_expr(expr->as.unary.expr, indent + 1);
      break;

    case MI_EXPR_BINARY:
      s_print_indent(indent);
      printf("%s BINARY(", can_fold);
      switch (expr->as.binary.op)
      {
        case MI_TOK_PLUS:   printf("+");  break;
        case MI_TOK_MINUS:  printf("-");  break;
        case MI_TOK_STAR:   printf("*");  break;
        case MI_TOK_SLASH:  printf("/");  break;
        case MI_TOK_EQEQ:   printf("=="); break;
        case MI_TOK_BANGEQ: printf("!="); break;
        case MI_TOK_LT:     printf("<");  break;
        case MI_TOK_GT:     printf(">");  break;
        case MI_TOK_LTEQ:   printf("<="); break;
        case MI_TOK_GTEQ:   printf(">="); break;
        case MI_TOK_AND:    printf("and");break;
        case MI_TOK_OR:     printf("or"); break;
        default:            printf("op?");break;
      }
      printf(")\n");
      mi_ast_debug_print_expr(expr->as.binary.left,  indent + 1);
      mi_ast_debug_print_expr(expr->as.binary.right, indent + 1);
      break;

    case MI_EXPR_LIST:
      {
        s_print_indent(indent);
        printf("%s LIST:\n", can_fold);
        MiExprList *it = expr->as.list.items;
        while (it)
        {
          mi_ast_debug_print_expr(it->expr, indent + 1);
          it = it->next;
        }
        break;
      }

    case MI_EXPR_DICT:
      {
        s_print_indent(indent);
        printf("%s DICT:\n", can_fold);
        MiExprList *it = expr->as.dict.items;
        while (it)
        {
          mi_ast_debug_print_expr(it->expr, indent + 1);
          it = it->next;
        }
        break;
      }

    case MI_EXPR_PAIR:
      s_print_indent(indent);
      printf("PAIR:\n");
      s_print_indent(indent + 1);
      printf("KEY:\n");
      mi_ast_debug_print_expr(expr->as.pair.key, indent + 2);
      s_print_indent(indent + 1);
      printf("VALUE:\n");
      mi_ast_debug_print_expr(expr->as.pair.value, indent + 2);
      break;

    case MI_EXPR_BLOCK:
      s_print_indent(indent);
      printf("%s BLOCK:\n", can_fold);
      s_print_indent(indent);
      printf("{\n");
      mi_ast_debug_print_script_indent(expr->as.block.script, indent + 1);
      s_print_indent(indent);
      printf("}\n");
      break;

    case MI_EXPR_COMMAND:
      {
        s_print_indent(indent);
        printf("%s COMMAND-EXPR HEAD:\n", can_fold);
        mi_ast_debug_print_expr(expr->as.command.head, indent + 1);
        s_print_indent(indent);
        printf("ARGS:\n");
        MiExprList *it = expr->as.command.args;
        while (it)
        {
          mi_ast_debug_print_expr(it->expr, indent + 1);
          it = it->next;
        }
        break;
      }

    default:
      s_print_indent(indent);
      printf("<unknown-expr-kind>\n");
      break;
  }
}

void mi_ast_debug_print_command(const MiCommand *cmd, int indent)
{
  if (!cmd)
  {
    s_print_indent(indent);
    printf("<null-command>\n");
    return;
  }

  s_print_indent(indent);
  printf("COMMAND:\n");

  s_print_indent(indent + 1);
  printf("HEAD:\n");
  mi_ast_debug_print_expr(cmd->head, indent + 2);

  int argc = 0;
  s_print_indent(indent + 1);
  {
    // count args
    MiExprList *count_it = cmd->args;
    while (count_it)
    {
      count_it = count_it->next;
      argc++;
    }
  }

  printf("ARGS (%d): \n", argc);

  MiExprList *it = cmd->args;
  if (!it)
  {
    s_print_indent(indent + 2);
    printf("<none>\n");
  }

  argc = 0;
  while (it)
  {
    s_print_indent(indent + 1);
    printf("ARG %d:\n", argc++);
    mi_ast_debug_print_expr(it->expr, indent + 2);
    it = it->next;
  }
}

void mi_ast_debug_print_script_indent(const MiScript *script, int indent)
{
  if (!script)
  {
    printf("<null-script>\n");
    return;
  }

  const MiCommandList *it = script->first;
  size_t index = 0;

  while (it)
  {
    s_print_indent(indent);
    printf("=== COMMAND %zu ===\n", index);
    mi_ast_debug_print_command(it->command, indent + 1);
    it = it->next;
    index = index + 1;
  }
}

void mi_ast_debug_print_script(const MiScript *script)
{
  mi_ast_debug_print_script_indent(script, 0);
}

#endif // defined(_DEBUG) || defined(DEBUG)

MiParseResult mi_parse_program(const char *source,
    size_t      source_len,
    XArena     *arena)
{
  MiParseResult result;
  result.ok            = false;
  result.script        = NULL;
  result.error_line    = 0;
  result.error_column  = 0;
  result.error_message = x_slice_empty();

  MiParser p;
  s_parser_init(&p, source, source_len, arena);

  MiScript *script = s_parse_script(&p, false);

  if (p.had_error)
  {
    result.ok           = false;
    result.script       = NULL;
    result.error_line   = p.error_line;
    result.error_column = p.error_column;
    result.error_message = p.error_message;
    return result;
  }

  result.ok     = true;
  result.script = script;
  return result;
}
