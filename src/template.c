#include "template.h"

static void s_x_ssg_escape_and_append_literal(XStrBuilder* out, const char* text, size_t len)
{
  size_t i;

  for (i = 0; i < len; ++i)
  {
    char c = text[i];

    if(c == '"')
    {
      x_strbuilder_append_cstr(out, "\\\"");
      continue;
    }

    x_strbuilder_append_char(out, c);
  }
}

static void s_x_ssg_emit_print_literal_segment(XStrBuilder* out, const char* start, size_t len)
{
  if (len == 0)
  {
    return;
  }

  x_strbuilder_append(out, "print :: \"");
  s_x_ssg_escape_and_append_literal(out, start, len);
  //x_strbuilder_append_substring(out, start, len);
  x_strbuilder_append(out, "\"\n");
}

/* Scanner helpers */

static char s_x_ssg_peek(const char* p, const char* end)
{
  if (p >= end)
  {
    return '\0';
  }
  return *p;
}

static char s_x_ssg_peek2(const char* p, const char* end)
{
  if ((p + 1) >= end)
  {
    return '\0';
  }
  return p[1];
}

static void s_x_ssg_advance(const char** p, const char* end, int* line, int* col)
{
  const char* cur = *p;
  if (cur >= end)
  {
    return;
  }

  char c = *cur;
  if (c == '\n')
  {
    *line += 1;
    *col   = 1;
  }
  else
  {
    *col += 1;
  }

  *p = cur + 1;
}

/* After "<%", skip spaces/tabs/CR and then a single '\n' (if present)
 * inside the code block. This removes the "empty first line" of code.
 */
static void s_x_ssg_skip_code_leading_ws_newline(const char** p, const char* end, int* line, int* col)
{
  const char* cur = *p;
  int         l   = *line;
  int         c   = *col;

  while (cur < end)
  {
    char ch = *cur;
    if (ch == ' ' || ch == '\t' || ch == '\r')
    {
      cur += 1;
      c   += 1;
    }
    else
    {
      break;
    }
  }

  if (cur < end && *cur == '\n')
  {
    cur += 1;
    l   += 1;
    c    = 1;
  }

  *p    = cur;
  *line = l;
  *col  = c;
}

/* After "%>", skip spaces/tabs/CR only if they are followed by '\n',
 * then skip that '\n'. This removes the "code-only" line but keeps
 * indentation of the next HTML line intact.
 */
static void s_x_ssg_skip_text_leading_ws_newline(const char** p, const char* end, int* line, int* col)
{
  const char* cur      = *p;
  int         l        = *line;
  int         c        = *col;
  const char* ws_start = cur;
  int         c_ws     = c;

  /* Tentatively skip spaces/tabs/CR */
  while (cur < end)
  {
    char ch = *cur;
    if (ch == ' ' || ch == '\t' || ch == '\r')
    {
      cur += 1;
      c_ws += 1;
    }
    else
    {
      break;
    }
  }

  /* If not followed by newline, do nothing at all */
  if (!(cur < end && *cur == '\n'))
  {
    return;
  }

  /* We have ws* + '\n': consume both */
  cur   += 1;     /* skip '\n' */
  l     += 1;
  c_ws   = 1;

  *p    = cur;
  *line = l;
  *col  = c_ws;

  (void)ws_start; /* silence unused warning if any compiler complains */
}

/* Flush text from segment_start..segment_end, but if the last line
 * in this range contains only indentation (spaces/tabs/CR) before "<%",
 * drop that indentation tail.
 *
 * Example segment before "<%":
 *   "  <head>\n  "
 * We want to emit only "  <head>\n".
 */
static void s_x_ssg_flush_text_segment_trim_indent(
    XStrBuilder* out,
    const char* segment_start,
    const char* segment_end
    )
{
  const char* flush_end = segment_end;

  if (segment_end > segment_start)
  {
    const char* line_start = segment_start;
    const char* t          = segment_end;

    // Find start of the last line in [segment_start, segment_end)
    while (t > segment_start)
    {
      if (t[-1] == '\n')
      {
        line_start = t;
        break;
      }
      t--;
    }

    // Check if line_start..segment_end is only horizontal whitespace
    {
      const char* q       = line_start;
      int         only_ws = 1;

      while (q < segment_end)
      {
        char ch = *q;
        if (ch != ' ' && ch != '\t' && ch != '\r')
        {
          only_ws = 0;
          break;
        }
        q++;
      }

      if (only_ws)
      {
        flush_end = line_start;
      }
    }
  }

  if (flush_end > segment_start)
  {
    s_x_ssg_emit_print_literal_segment(out, segment_start, (size_t)(flush_end - segment_start));
  }
}

MISsgResult ssg_expand_minima_template(XSlice input, XStrBuilder* out)
{
  typedef enum SsgMode
  {
    SSG_MODE_TEXT = 0,
    SSG_MODE_CODE = 1
  } SsgMode;

  MISsgResult result;
  const char* ptr           = input.ptr;
  const char* end           = input.ptr + input.length;
  const char* p             = ptr;
  const char* segment_start = ptr;

  int line = 1;
  int col  = 1;

  int block_start_line   = 0;
  int block_start_column = 0;

  SsgMode mode = SSG_MODE_TEXT;
  int     is_expr_block = 0; /* 1 for <%= ... %> blocks */

  x_smallstr_clear(&result.error_message);
  result.error_code   = 0;
  result.error_line   = 0;
  result.error_column = 0;

  while (p < end)
  {
    char c = s_x_ssg_peek(p, end);

    if (mode == SSG_MODE_TEXT)
    {
      if (c == '<' && s_x_ssg_peek2(p, end) == '%')
      {
        /* Distinguish <% ... %> vs <%= ... %> */
        int is_expr = 0;

        if ((p + 2) < end && p[2] == '=')
        {
          is_expr = 1;
        }

        /* Flush previous literal text, trimming indentation-only tail */
        if (p > segment_start)
        {
          s_x_ssg_flush_text_segment_trim_indent(out, segment_start, p);
        }

        block_start_line   = line;
        block_start_column = col;
        is_expr_block      = is_expr;

        if (is_expr_block)
        {
          // Consume "<%="
          s_x_ssg_advance(&p, end, &line, &col);
          s_x_ssg_advance(&p, end, &line, &col);
          s_x_ssg_advance(&p, end, &line, &col);

          mode = SSG_MODE_CODE;

          /* Skip optional leading ws+newline inside expr block */
          s_x_ssg_skip_code_leading_ws_newline(&p, end, &line, &col);

          // Emit prefix for expression: ( print :: ... ) 
          x_strbuilder_append_cstr(out, "( print :: ");

          // Expression content starts here
          segment_start = p;
          continue;
        }
        else
        {
          // Normal "<%" block
          // Consume "<%"
          s_x_ssg_advance(&p, end, &line, &col);
          s_x_ssg_advance(&p, end, &line, &col);

          mode = SSG_MODE_CODE;

          // Skip optional leading ws+newline inside code block
          s_x_ssg_skip_code_leading_ws_newline(&p, end, &line, &col);

          // Code content starts here
          segment_start = p;
          continue;
        }
      }

      // Regular text character
      s_x_ssg_advance(&p, end, &line, &col);
    }
    else // SSG_MODE_CODE
    {
      if (c == '%' && s_x_ssg_peek2(p, end) == '>')
      {
        if (is_expr_block)
        {
          /* For <%= ... %> we already emitted "( print :: ".
           * Now emit the expression text, then close with ")" and newline.
           */
          if (p > segment_start)
          {
            x_strbuilder_append_substring(out, segment_start, (size_t)(p - segment_start));
          }

          x_strbuilder_append_cstr(out, ")\n");
        }
        else
        {
          // Normal <% ... %> block: flush as-is, ensure newline if needed
          if (p > segment_start)
          {
            size_t len = (size_t)(p - segment_start);

            x_strbuilder_append_substring(out, segment_start, len);

            if (segment_start[len - 1] != '\n')
            {
              x_strbuilder_append_cstr(out, "\n");
            }
          }
        }

        // Consume "%>"
        s_x_ssg_advance(&p, end, &line, &col);
        s_x_ssg_advance(&p, end, &line, &col);

        // In template text, skip ws only if followed by newline, then newline
        s_x_ssg_skip_text_leading_ws_newline(&p, end, &line, &col);

        mode          = SSG_MODE_TEXT;
        segment_start = p;
        is_expr_block = 0;
        continue;
      }

      /* Regular code character */
      s_x_ssg_advance(&p, end, &line, &col);
    }
  }

  if (mode == SSG_MODE_TEXT)
  {
    if (p > segment_start)
    {
      s_x_ssg_emit_print_literal_segment(out, segment_start, (size_t)(p - segment_start));
    }
  }
  else
  {
    result.error_code   = 1;
    result.error_line   = block_start_line;
    result.error_column = block_start_column;

    x_smallstr_appendf(
        &result.error_message,
        "Unterminated <%% %%> block (started at line %d, column %d)",
        block_start_line,
        block_start_column
        );
  }

  return result;
}
