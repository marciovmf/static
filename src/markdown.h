/*
   md.h — C99 progressive non-regex Markdown to HTML 

   Overview
   --------
   Markdown is deceptively simple until you try to parse it for real.
   Every edge case (nested lists, backticks, emphasis ambiguity, link titles)
   drags you toward a full grammar engine—something Markdown never had to begin with.
   This implementation takes a pragmatic, layered approach: a series of explicit,
   ordered passes that progressively rewrite a Markdown document into HTML,
   without using regexes or dynamic parser generators.

   The design accepts imperfection as a feature. It doesn’t aim for complete
   CommonMark compliance; instead, it focuses on consistency, predictability,
   and human readability of the output. The result is a parser that covers
   roughly 95% of everyday Markdown without any external dependencies, and
   whose internal logic is readable, hackable, and debuggable in a single file.

   Approach
   --------
   The idea came from this post where someone just applied a series of successive
   regex substitutions to minimaly convert Markdown to HTML.
   (https://dev.to/casualwriter/a-simple-markdown-parser-in-50-lines-of-js-4gpi)
   Turns out this is "good enough" for my purposes.

   The parser processes text in sequential phases, each making one kind of
   structural decision and passing the result forward. This mirrors how a
   human visually parses Markdown—recognizing code fences before thinking
   about paragraphs, for example.

   • Phase 0 — Normalize newlines
   Convert CRLF and CR into '\n'.  No further processing assumes Windows line endings.

   • Phase 1 — Fence and code extraction
   Splits the document into TEXT and CODE chunks based on ``` or ~~~ fences.
   These are emitted verbatim later as <pre><code> blocks and skipped during
   inline/structural parsing, isolating code content from further rewriting.

   • Phase 2 — TEXT block rendering
   Each TEXT chunk is processed line by line.
   - Detects headers (#..#####)
   - Horizontal rules (---, ***, ___)
   - Blockquotes (> and >>)
   - Lists (unordered: -, +, *, ordered: 1. 2. ...)
   - Indented code (4+ spaces or tabs)
   - Paragraphs and soft line breaks
   - Inline syntax: escapes, code spans, links, images, emphasis, deletion, insertion

   • Phase 3 — Merge and emit
   Concatenates rendered TEXT and CODE chunks into final HTML.
   Adjacent fenced code blocks with the same type are coalesced for compactness.

   Implementation Notes
   --------------------
   - Inline parsing uses literal scanning and balanced-marker search.
   No regex engine is used; everything is done with simple loops and pointer arithmetic.
   - The order of inline rules matters: escapes → code spans → images/links →
   strong/emphasis → deletions/insertions → raw text.
   This ordering minimizes ambiguity between overlapping markers.
   - Links and images use a strict "URL before title" rule.
   The URL ends at the first whitespace leading to a quoted title, if present.
   Escaped quotes inside titles (\"foo\") are supported.
   - Autolinks (<http://...>) are handled as a special case.
   - Lists are parsed by indentation width and bullet markers, not by
   recursive descent.  This works for all "normal" Markdown, though
   pathological mixes of bullets and numbers at varying indents may behave
   differently than CommonMark.

   Limitations (and why that’s fine)
   ---------------------------------
   Markdown is not a real grammar. It’s a social contract with many dialects.
   The philosophy here is “parse what’s obvious, escape what’s not.”
   Anything inside backticks or fences is sacred. Everything else is fair game.

   In short, it transforms the most useful and visually consistent subset of Markdown
   into safe HTML without the usual chaos or regex nightmares.

   Final considerations
   ---------------------
   - I hate parsing Markdown.
   - F*37c Markdown.
*/

#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_strbuilder.h>
#include <stdx_arena.h>
#include <stdbool.h>

char* md_to_html(const char *markdown);

#ifdef MD_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Str
{
  XStrBuilder* sb;
} Str;

static void str_init(Str *s)
{
  s->sb = x_strbuilder_create();
}

static void str_free(Str *s)
{
  if (s->sb)
  {
    x_strbuilder_destroy(s->sb);
    s->sb = NULL;
  }
}

static void str_putc(Str *s, char c)
{
  if (!s->sb) return;
  x_strbuilder_append_char(s->sb, c);
}

static void str_putn(Str *s, const char *p, size_t n)
{
  if (!s->sb || !p || n == 0) return;
  x_strbuilder_append_substring(s->sb, p, n);
}

static void str_puts(Str *s, const char *p)
{
  if (!s->sb || !p) return;
  x_strbuilder_append(s->sb, p);
}

static void str_clear(Str *s)
{
  if (!s->sb) return;
  x_strbuilder_clear(s->sb);
}

static size_t str_length(const Str *s)
{
  if (!s || !s->sb) return 0;
  return x_strbuilder_length((XStrBuilder*)s->sb);
}

static char* str_data(const Str *s)
{
  if (!s || !s->sb) return NULL;
  return x_strbuilder_to_string((const XStrBuilder*)s->sb);
}

static void str_append_str(Str *dst, const Str *src)
{
  if (!dst || !dst->sb || !src || !src->sb) return;
  char *data = x_strbuilder_to_string((const XStrBuilder*)src->sb);
  size_t len = x_strbuilder_length((XStrBuilder*)src->sb);
  if (len > 0 && data)
  {
    x_strbuilder_append_substring(dst->sb, data, len);
  }
}

static void html_escape(Str *s, const char *p, size_t n)
{
  size_t i = 0;
  for (i = 0; i < n; ++i)
  {
    char c = p[i];
    if (c == '&') str_puts(s, "&amp;");
    else if (c == '<') str_puts(s, "&lt;");
    else if (c == '>') str_puts(s, "&gt;");
    else str_putc(s, c);
  }
}

static void attr_escape(Str *s, const char *p, size_t n)
{
  size_t i = 0;
  for (i = 0; i < n; ++i)
  {
    char c = p[i];
    if (c == '&') str_puts(s, "&amp;");
    else if (c == '<') str_puts(s, "&lt;");
    else if (c == '>') str_puts(s, "&gt;");
    else if (c == '\"') str_puts(s, "&quot;");
    else if (c == '\\' && i + 1 < n && p[i + 1] == '\"')
    {
      str_puts(s, "&quot;");
      i += 1;
    }
    else str_putc(s, c);
  }
}

typedef struct Src
{
  const char *p;
  const char *end;
  size_t line;
} Src;

static void src_init(Src *s, const char *text)
{
  s->p = text;
  s->end = text + strlen(text);
  s->line = 1;
}

static bool src_eof(const Src *s)
{
  return s->p >= s->end;
}

static const char *src_line_end(const char *p, const char *end)
{
  const char *q = p;
  while (q < end && *q != '\n') q++;
  return q;
}

static bool starts_with(const char *p, const char *end, const char *lit)
{
  size_t n = strlen(lit);
  if ((size_t)(end - p) < n) return false;
  return memcmp(p, lit, n) == 0;
}

// Phase 0
static char *md_normalize_newlines(const char *in)
{
  size_t n = strlen(in);
  char *out = (char*)malloc(n + 1);
  if (!out) return NULL;
  size_t j = 0;
  size_t i = 0;
  for (i = 0; i < n; ++i)
  {
    char c = in[i];
    if (c == '\r')
    {
      if (i + 1 < n && in[i + 1] == '\n')
      {
        // skip CR in CRLF
      }
      else
      {
        out[j] = '\n';
        j += 1;
      }
    }
    else
    {
      out[j] = c;
      j += 1;
    }
  }
  out[j] = '\0';
  return out;
}

// Phase 1: split into text and code chunks
typedef enum
{
  MD_CHUNK_TEXT,
  MD_CHUNK_CODE
} MDChunkKind;

typedef struct
{
  MDChunkKind kind;
  const char *begin;
  const char *end;
  const char *info_begin;
  const char *info_end;
} MDChunk;

typedef struct
{
  MDChunk *v;
  size_t n;
  size_t cap;
} MDChunks;

static void chunks_push(MDChunks *cs, MDChunk c)
{
  if (cs->n == cs->cap)
  {
    size_t nc = cs->cap ? cs->cap * 2 : 16;
    MDChunk *nv = (MDChunk*)realloc(cs->v, nc * sizeof(*nv));
    if (!nv) abort();
    cs->v = nv;
    cs->cap = nc;
  }
  cs->v[cs->n] = c;
  cs->n += 1;
}

static void md_split_fences(const char *buf, MDChunks *out)
{
  Src s;
  src_init(&s, buf);
  const char *cursor = s.p;
  while (!src_eof(&s))
  {
    const char *line = s.p;
    const char *eol = src_line_end(s.p, s.end);
    bool fence = false;
    size_t fence_len = 0;

    const char *p = line;
    while (p < eol && (*p == ' ' || *p == '\t')) p++;
    const char *f = p;
    while (f < eol && (*f == '`' || *f == '~')) f++;
    if (f > p && (size_t)(f - p) >= 3)
    {
      fence = true;
      fence_len = (size_t)(f - p);
    }

    if (!fence)
    {
      s.p = (eol < s.end) ? eol + 1 : eol;
      continue;
    }

    if (cursor < line)
    {
      MDChunk ctext;
      ctext.kind = MD_CHUNK_TEXT;
      ctext.begin = cursor;
      ctext.end = line;
      ctext.info_begin = NULL;
      ctext.info_end = NULL;
      chunks_push(out, ctext);
    }

    const char *info_begin = f;
    while (info_begin < eol && (*info_begin == ' ' || *info_begin == '\t')) info_begin++;
    const char *info_end = eol;
    while (info_end > info_begin && (info_end[-1] == ' ' || info_end[-1] == '\t')) info_end--;

    char tick = *p;
    const char *body_begin = (eol < s.end) ? eol + 1 : eol;
    const char *q = body_begin;
    const char *body_end = q;

    for (;;)
    {
      if (q >= s.end)
      {
        body_end = s.end;
        break;
      }
      const char *qeol = src_line_end(q, s.end);
      const char *r = q;
      while (r < qeol && (*r == ' ' || *r == '\t')) r++;
      const char *u = r;
      while (u < qeol && *u == tick) u++;
      if ((size_t)(u - r) >= fence_len)
      {
        body_end = q;
        s.p = (qeol < s.end) ? qeol + 1 : qeol;
        break;
      }
      q = (qeol < s.end) ? qeol + 1 : qeol;
    }

    MDChunk code;
    code.kind = MD_CHUNK_CODE;
    code.begin = body_begin;
    code.end = body_end;
    code.info_begin = info_begin;
    code.info_end = info_end;
    chunks_push(out, code);

    cursor = s.p;
  }

  if (cursor < s.end)
  {
    MDChunk ctail;
    ctail.kind = MD_CHUNK_TEXT;
    ctail.begin = cursor;
    ctail.end = s.end;
    ctail.info_begin = NULL;
    ctail.info_end = NULL;
    chunks_push(out, ctail);
  }
}

// Helpers
static bool is_all_same_char(const char *p, const char *end, char ch)
{
  if (p >= end) return false;
  const char *q = p;
  for (; q < end; ++q)
  {
    if (*q != ch) return false;
  }
  return true;
}

static const char *skip_ws(const char *p, const char *end)
{
  while (p < end && (*p == ' ' || *p == '\t')) p++;
  return p;
}

// Headers
static void render_header_line(Str *out, const char *line, const char *eol)
{
  const char *p = line;
  int level = 0;
  while (p < eol && *p == '#')
  {
    level += 1;
    p += 1;
  }
  if (level < 1 || level > 5) return;
  if (p < eol && *p == ' ') p += 1;

  const char *text0 = p;
  const char *rt = eol;
  while (rt > text0 && (rt[-1] == ' ' || rt[-1] == '\t' || rt[-1] == '#')) rt--;

  XSmallstr slug;
  x_smallstr_clear(&slug);
  bool prev_dash = false;
  const char *qq = text0;
  for (; qq < rt; ++qq)
  {
    unsigned char c = (unsigned char)*qq;
    if (isalnum(c))
    {
      x_smallstr_append_char(&slug, (char)tolower(c));
      prev_dash = false;
    }
    else if (c == ' ' || c == '-' || c == '_')
    {
      if (!prev_dash)
      {
        x_smallstr_append_char(&slug, '-');
        prev_dash = true;
      }
    }
    else
    {
      if (!prev_dash)
      {
        x_smallstr_append_char(&slug, '-');
        prev_dash = true;
      }
    }
  }
  while (slug.length > 0 && slug.buf[slug.length - 1] == '-')
  {
    slug.length -= 1;
    slug.buf[slug.length] = '\0';
  }

  char tag[8];
  snprintf(tag, sizeof(tag), "h%d", level);

  str_putc(out, '<');
  str_puts(out, tag);
  if (slug.length > 0)
  {
    str_puts(out, " id=\"");
    attr_escape(out, slug.buf, slug.length);
    str_puts(out, "\"");
  }
  str_puts(out, ">");
  html_escape(out, text0, (size_t)(rt - text0));
  str_puts(out, "</");
  str_puts(out, tag);
  str_puts(out, ">\n");

}

// Strict URL scanner
static const char *scan_url_and_maybe_title(const char *p, const char *end,
    const char **url0, const char **url1,
    const char **title0, const char **title1,
    const char **close_paren)
{
  const char *u0 = p;
  const char *u1 = NULL;
  const char *t0 = NULL;
  const char *t1 = NULL;
  const char *cp = NULL;

  const char *first_ws = NULL;
  int paren = 0;
  const char *i = p;

  while (i < end)
  {
    char c = *i;

    if (c == '\\' && i + 1 < end)
    {
      i += 2;
      continue;
    }

    if (c == '(')
    {
      paren += 1;
      i += 1;
      continue;
    }

    if (isspace((unsigned char)c))
    {
      if (!first_ws) first_ws = i;
      const char *sp = i;
      while (sp < end && isspace((unsigned char)*sp)) sp++;
      if (sp < end && *sp == '\"')
      {
        u1 = first_ws ? first_ws : i;
        const char *k = sp + 1;
        while (k < end)
        {
          if (*k == '\\' && k + 1 < end)
          {
            k += 2;
            continue;
          }
          if (*k == '\"')
          {
            t0 = sp + 1;
            t1 = k;
            k += 1;
            break;
          }
          k += 1;
        }
        while (k < end && isspace((unsigned char)*k)) k++;
        if (k < end && *k == ')') cp = k;
        else
        {
          const char *m = k;
          while (m < end && *m != ')') m++;
          if (m < end) cp = m;
        }
        break;
      }
      i += 1;
      continue;
    }

    if (c == ')')
    {
      if (paren == 0)
      {
        cp = i;
        if (!u1) u1 = i;
        break;
      }
      paren -= 1;
      i += 1;
      continue;
    }

    i += 1;
  }

  if (!u1) u1 = (i < end ? i : end);

  *url0 = u0;
  *url1 = u1;
  *title0 = t0;
  *title1 = t1;
  *close_paren = cp;

  return cp ? cp : i;
}

// Inline rendering
static void render_inline(Str *out, const char *p, const char *end)
{
  while (p < end)
  {
    // escapes
    if (*p == '\\' && p + 1 < end)
    {
      str_putc(out, p[1]);
      p += 2;
      continue;
    }

    // code spans ``...`` or `...`
    if (*p == '`')
    {
      const char *q = p + 1;
      bool dbl = (q < end && *q == '`');
      if (dbl) q += 1;
      const char *close = NULL;
      while (q < end)
      {
        if (*q == '`')
        {
          if (dbl)
          {
            if (q + 1 < end && q[1] == '`') { close = q; break; }
          }
          else { close = q; break; }
        }
        q += 1;
      }
      if (close)
      {
        str_puts(out, "<code>");
        html_escape(out, p + (dbl ? 2 : 1), (size_t)(close - (p + (dbl ? 2 : 1))));
        str_puts(out, "</code>");
        p = close + (dbl ? 2 : 1);
        continue;
      }
    }

    // images
    if (*p == '!' && p + 1 < end && p[1] == '[')
    {
      const char *a0 = p + 2;
      const char *a1 = a0;
      while (a1 < end && *a1 != ']') a1++;
      if (a1 < end && a1 + 1 < end && a1[1] == '(')
      {
        const char *u0 = NULL;
        const char *u1 = NULL;
        const char *t0 = NULL;
        const char *t1 = NULL;
        const char *cp = NULL;
        scan_url_and_maybe_title(a1 + 2, end, &u0, &u1, &t0, &t1, &cp);
        if (cp && cp < end)
        {
          str_puts(out, "<img alt=\"");
          attr_escape(out, a0, (size_t)(a1 - a0));
          str_puts(out, "\" src=\"");
          attr_escape(out, u0, (size_t)(u1 - u0));
          str_puts(out, "\"");
          if (t0 && t1)
          {
            str_puts(out, " title=\"");
            attr_escape(out, t0, (size_t)(t1 - t0));
            str_puts(out, "\"");
          }
          str_puts(out, " />");
          p = cp + 1;
          continue;
        }
      }
    }

    // links
    if (*p == '[')
    {
      const char *t0 = p + 1;
      const char *t1 = t0;
      while (t1 < end && *t1 != ']') t1++;
      if (t1 < end && t1 + 1 < end && t1[1] == '(')
      {
        const char *u0 = NULL;
        const char *u1 = NULL;
        const char *title0 = NULL;
        const char *title1 = NULL;
        const char *cp = NULL;
        scan_url_and_maybe_title(t1 + 2, end, &u0, &u1, &title0, &title1, &cp);
        if (cp && cp < end)
        {
          if (u0 == u1)
          {
            u0 = t0;
            u1 = t1;
          }
          str_puts(out, "<a href=\"");
          attr_escape(out, u0, (size_t)(u1 - u0));
          str_puts(out, "\"");
          if (title0 && title1)
          {
            str_puts(out, " title=\"");
            attr_escape(out, title0, (size_t)(title1 - title0));
            str_puts(out, "\"");
          }
          str_puts(out, ">");
          html_escape(out, t0, (size_t)(t1 - t0));
          str_puts(out, "</a>");
          p = cp + 1;
          continue;
        }
      }
    }

    // autolink <http...>
    if (starts_with(p, end, "<http"))
    {
      const char *q2 = p + 1;
      while (q2 < end && *q2 != '>') q2++;
      if (q2 < end)
      {
        const char *u = p + 1;
        str_puts(out, "<a href=\"");
        attr_escape(out, u, (size_t)(q2 - u));
        str_puts(out, "\">");
        html_escape(out, u, (size_t)(q2 - u));
        str_puts(out, "</a>");
        p = q2 + 1;
        continue;
      }
    }

    // strong+em
    if ((p + 5) < end && ((p[0] == '*' && p[1] == '*' && p[2] == '*') || (p[0] == '_' && p[1] == '_' && p[2] == '_')))
    {
      char ch = p[0];
      const char *q3 = p + 3;
      const char *close3 = NULL;
      while (q3 + 2 < end)
      {
        if (q3[0] == ch && q3[1] == ch && q3[2] == ch) { close3 = q3; break; }
        q3 += 1;
      }
      if (close3)
      {
        str_puts(out, "<b><em>");
        html_escape(out, p + 3, (size_t)(close3 - (p + 3)));
        str_puts(out, "</em></b>");
        p = close3 + 3;
        continue;
      }
    }

    // strong or underline
    if ((p + 3) < end && ((p[0] == '*' && p[1] == '*') || (p[0] == '_' && p[1] == '_')))
    {
      char ch2 = p[0];
      const char *q4 = p + 2;
      const char *close4 = NULL;
      while (q4 + 1 < end)
      {
        if (q4[0] == ch2 && q4[1] == ch2) { close4 = q4; break; }
        q4 += 1;
      }
      if (close4)
      {
        if (ch2 == '*') str_puts(out, "<b>");
        else str_puts(out, "<u>");
        html_escape(out, p + 2, (size_t)(close4 - (p + 2)));
        if (ch2 == '*') str_puts(out, "</b>");
        else str_puts(out, "</u>");
        p = close4 + 2;
        continue;
      }
    }

    // em
    if ((p + 2) < end && (*p == '*' || *p == '_'))
    {
      char ch3b = *p;
      const char *q5 = p + 1;
      const char *close5 = NULL;
      while (q5 < end)
      {
        if (*q5 == ch3b) { close5 = q5; break; }
        q5 += 1;
      }
      if (close5)
      {
        str_puts(out, "<em>");
        html_escape(out, p + 1, (size_t)(close5 - (p + 1)));
        str_puts(out, "</em>");
        p = close5 + 1;
        continue;
      }
    }

    // del
    if ((p + 3) < end && p[0] == '~' && p[1] == '~')
    {
      const char *q6 = p + 2;
      const char *close6 = NULL;
      while (q6 + 1 < end)
      {
        if (q6[0] == '~' && q6[1] == '~') { close6 = q6; break; }
        q6 += 1;
      }
      if (close6)
      {
        str_puts(out, "<del>");
        html_escape(out, p + 2, (size_t)(close6 - (p + 2)));
        str_puts(out, "</del>");
        p = close6 + 2;
        continue;
      }
    }

    // ins
    if ((p + 3) < end && p[0] == '^' && p[1] == '^')
    {
      const char *q7 = p + 2;
      const char *close7 = NULL;
      while (q7 + 1 < end)
      {
        if (q7[0] == '^' && q7[1] == '^') { close7 = q7; break; }
        q7 += 1;
      }
      if (close7)
      {
        str_puts(out, "<ins>");
        html_escape(out, p + 2, (size_t)(close7 - (p + 2)));
        str_puts(out, "</ins>");
        p = close7 + 2;
        continue;
      }
    }

    // default
    if (*p == '&') str_puts(out, "&amp;");
    else if (*p == '<') str_puts(out, "&lt;");
    else if (*p == '>') str_puts(out, "&gt;");
    else str_putc(out, *p);
    p += 1;
  }
}

// Lists helpers
static int ind_width(const char *p, const char *eol)
{
  int w = 0;
  const char *q = p;
  while (q < eol)
  {
    if (*q == ' ') { w += 1; q++; }
    else if (*q == '\t') { w += 4; q++; }
    else break;
  }
  return w;
}

static bool parse_bullet(const char *p, const char *eol, bool *ordered, const char **after_marker)
{
  const char *q = p;
  if (q < eol && (*q == '-' || *q == '+' || *q == '*'))
  {
    if (q + 1 < eol && (q[1] == ' ' || q[1] == '.'))
    {
      *ordered = false;
      *after_marker = q + 2;
      return true;
    }
  }
  if (q < eol && isdigit((unsigned char)*q))
  {
    while (q < eol && isdigit((unsigned char)*q)) q++;
    if (q < eol && (*q == '.' || *q == ' '))
    {
      *ordered = true;
      *after_marker = q + 1;
      return true;
    }
  }
  return false;
}

typedef struct ListStack
{
  bool kind[64];
  int top;
} ListStack;

static void ls_init(ListStack *s)
{
  s->top = 0;
}

static void ls_push(ListStack *s, bool ordered)
{
  if (s->top < 64)
  {
    s->kind[s->top] = ordered;
    s->top += 1;
  }
}

static bool ls_pop(ListStack *s, bool *ordered)
{
  if (s->top == 0) return false;
  s->top -= 1;
  *ordered = s->kind[s->top];
  return true;
}

static void close_lists_to(Str *out, ListStack *st, int *depth_ref, int target_depth)
{
  while (*depth_ref > target_depth)
  {
    str_puts(out, "</li>");
    bool ord = false;
    if (ls_pop(st, &ord))
    {
      if (ord) str_puts(out, "</ol>");
      else str_puts(out, "</ul>");
    }
    *depth_ref -= 1;
  }
}

static void open_list(Str *out, ListStack *st, int *depth_ref, bool ordered)
{
  if (ordered) str_puts(out, "<ol>");
  else str_puts(out, "<ul>");
  ls_push(st, ordered);
  *depth_ref += 1;
}

static void render_indented_code(Str *out, const char **pp, const char *block_end)
{
  const char *p = *pp;
  bool any = false;
  Str body;
  str_init(&body);
  while (p < block_end)
  {
    const char *eol = src_line_end(p, block_end);
    const char *q = p;
    int spaces = 0;
    while (q < eol && *q == ' ' && spaces < 10) { spaces += 1; q += 1; }
    bool tab = (p < eol && *p == '\t');
    if (!(tab || spaces >= 4)) break;
    any = true;
    if (tab) q = p + 1;
    html_escape(&body, q, (size_t)(eol - q));
    str_putc(&body, '\n');
    p = (eol < block_end) ? eol + 1 : eol;
    const char *ne = src_line_end(p, block_end);
    const char *rt = ne;
    while (rt > p && (rt[-1] == ' ' || rt[-1] == '\t')) rt--;
    if (rt == p) break;
  }
  if (any)
  {
    str_puts(out, "<pre><code>");
    str_append_str(out, &body);
    str_puts(out, "</code></pre>\n");
    *pp = p;
  }
  str_free(&body);
}

static void render_list_group(Str *out, const char **pp, const char *block_end)
{
  const char *p = *pp;
  const char *eol = src_line_end(p, block_end);
  bool ord = false;
  const char *after = NULL;
  if (!parse_bullet(p, eol, &ord, &after)) return;

  int base = ind_width(p, eol);
  ListStack st;
  ls_init(&st);
  int depth = 0;

  open_list(out, &st, &depth, ord);
  str_puts(out, "<li>");
  Str tmp;
  str_init(&tmp);
  render_inline(&tmp, after, eol);
  str_append_str(out, &tmp);
  str_free(&tmp);

  const char *cur = (eol < block_end) ? eol + 1 : eol;
  while (cur < block_end)
  {
    const char *le = src_line_end(cur, block_end);
    const char *rt = le;
    while (rt > cur && (rt[-1] == ' ' || rt[-1] == '\t')) rt--;
    if (rt == cur)
    {
      cur = (le < block_end) ? le + 1 : le;
      break;
    }

    const char *q = cur;
    int ind = ind_width(q, le);
    q += ind;

    bool ord2 = false;
    const char *after2 = NULL;
    if (!parse_bullet(q, le, &ord2, &after2)) break;

    int rel = ind - base;
    if (rel < 0) rel = 0;
    int target_level = 1 + (rel / 2);

    if (target_level > depth)
    {
      while (depth < target_level)
      {
        open_list(out, &st, &depth, ord2);
        str_puts(out, "<li>");
      }
    }
    else if (target_level < depth)
    {
      close_lists_to(out, &st, &depth, target_level);
      str_puts(out, "</li>");
      str_puts(out, "<li>");
    }
    else
    {
      str_puts(out, "</li>");
      str_puts(out, "<li>");
    }

    Str t2;
    str_init(&t2);
    render_inline(&t2, after2, le);
    str_append_str(out, &t2);
    str_free(&t2);

    cur = (le < block_end) ? le + 1 : le;
  }

  close_lists_to(out, &st, &depth, 0);
  bool ord_close = false;
  if (ls_pop(&st, &ord_close))
  {
    str_puts(out, "</li>");
    if (ord_close) str_puts(out, "</ol>\n");
    else str_puts(out, "</ul>\n");
  }
  *pp = cur;
}

// Blockquotes and text block rendering
static void render_blockquote_group(Str *out, const char **pp, const char *block_end)
{
  const char *p = *pp;
  int open_depth = 0;
  bool started = false;

  while (p < block_end)
  {
    const char *eol = src_line_end(p, block_end);
    const char *q = p;
    int depth = 0;
    while (q < eol && *q == '>') { depth += 1; q += 1; }
    if (depth == 0 || (q < eol && *q != ' ' && *q != '>')) break;
    if (q < eol && *q == ' ') q += 1;

    if (!started)
    {
      open_depth = depth;
      int i = 0;
      for (i = 0; i < open_depth; ++i) str_puts(out, "<blockquote>");
      started = true;
    }
    if (depth != open_depth)
    {
      while (open_depth > depth) { str_puts(out, "</blockquote>"); open_depth -= 1; }
      while (open_depth < depth) { str_puts(out, "<blockquote>"); open_depth += 1; }
    }

    Str tmp;
    str_init(&tmp);
    render_inline(&tmp, q, eol);
    str_append_str(out, &tmp);
    str_free(&tmp);

    const char *next = (eol < block_end) ? eol + 1 : eol;
    if (next < block_end)
    {
      const char *neol = src_line_end(next, block_end);
      const char *r = next;
      int d2 = 0;
      while (r < neol && *r == '>') { d2 += 1; r += 1; }
      if (d2 > 0 && (r == neol || *r == ' ' || *r == '>'))
      {
        str_puts(out, "\n<br>");
      }
    }
    p = next;
  }

  if (started)
  {
    int i = 0;
    for (i = 0; i < open_depth; ++i) str_puts(out, "</blockquote>");
    str_putc(out, '\n');
  }

  *pp = p;
}

static void render_text_block(Str *out, const char *begin, const char *end)
{
  const char *p = begin;
  Str para;
  str_init(&para);
  bool in_para = false;

  while (p < end)
  {
    const char *eol = src_line_end(p, end);
    const char *rt = eol;
    while (rt > p && (rt[-1] == ' ' || rt[-1] == '\t')) rt--;

    bool is_blank = (rt == p);

    if (!is_blank && *p == '>')
    {
      if (in_para)
      {
        str_puts(out, "<p>");
        str_append_str(out, &para);
        str_puts(out, "</p>\n");
        str_clear(&para);
        in_para = false;
      }
      render_blockquote_group(out, &p, end);
      while (p < end)
      {
        const char *le = src_line_end(p, end);
        const char *q2 = p;
        int d = 0;
        while (q2 < le && *q2 == '>') { d += 1; q2 += 1; }
        if (d == 0 || (q2 < le && *q2 != ' ' && *q2 != '>')) break;
        if (q2 < le && *q2 == ' ') q2 += 1;
        str_puts(out, "<br>");
        render_blockquote_group(out, &p, end);
      }
      continue;
    }

    int indent = ind_width(p, eol);
    if (!is_blank && (*p == '\t' || (*p == ' ' && indent >= 4)))
    {
      const char *q3 = p;
      while (q3 < rt && *q3 == ' ') q3++;
      if (!(q3 < rt && (*q3 == '-' || *q3 == '+' || *q3 == '*' || isdigit((unsigned char)*q3))))
      {
        if (in_para)
        {
          str_puts(out, "<p>");
          str_append_str(out, &para);
          str_puts(out, "</p>\n");
          str_clear(&para);
          in_para = false;
        }
        render_indented_code(out, &p, end);
        continue;
      }
    }

    {
      const char *am = NULL;
      bool dummy_ord = false;
      bool is_list = false;
      if (!is_blank && parse_bullet(p, rt, &dummy_ord, &am)) is_list = true;
      if (is_list)
      {
        if (in_para)
        {
          str_puts(out, "<p>");
          str_append_str(out, &para);
          str_puts(out, "</p>\n");
          str_clear(&para);
          in_para = false;
        }
        render_list_group(out, &p, end);
        continue;
      }
    }

    bool is_hr = false;
    {
      const char *q4 = p;
      while (q4 < rt && (*q4 == ' ' || *q4 == '\t')) q4++;
      if (q4 < rt)
      {
        char ch = *q4;
        if (ch == '-' || ch == '*' || ch == '_')
        {
          is_hr = is_all_same_char(q4, rt, ch) && (rt - q4) >= 3;
        }
      }
    }
    if (is_hr)
    {
      if (in_para)
      {
        str_puts(out, "<p>");
        str_append_str(out, &para);
        str_puts(out, "</p>\n");
        str_clear(&para);
        in_para = false;
      }
      str_puts(out, "<hr/>\n");
      p = (eol < end) ? eol + 1 : eol;
      continue;
    }

    bool is_header = false;
    int header_hashes = 0;
    const char *qh = p;
    while (qh < eol && *qh == '#') { header_hashes += 1; qh += 1; }
    if (header_hashes >= 1 && header_hashes <= 5 && (qh < eol && *qh == ' ')) is_header = true;
    if (is_header)
    {
      if (in_para)
      {
        str_puts(out, "<p>");
        str_append_str(out, &para);
        str_puts(out, "</p>\n");
        str_clear(&para);
        in_para = false;
      }
      Str h;
      str_init(&h);
      render_header_line(&h, p, rt);
      str_append_str(out, &h);
      str_free(&h);
      p = (eol < end) ? eol + 1 : eol;
      continue;
    }

    if (is_blank)
    {
      if (in_para)
      {
        str_puts(out, "<p>");
        str_append_str(out, &para);
        str_puts(out, "</p>\n");
        str_clear(&para);
        in_para = false;
      }
      p = (eol < end) ? eol + 1 : eol;
      continue;
    }

    if (!in_para) in_para = true;
    else str_putc(&para, '\n');

    Str tmp2;
    str_init(&tmp2);
    render_inline(&tmp2, p, rt);
    str_append_str(&para, &tmp2);
    str_free(&tmp2);

    size_t trail_spaces = (size_t)(eol - rt);
    if (trail_spaces >= 1)
    {
      str_puts(&para, "<br/>");
    }

    p = (eol < end) ? eol + 1 : eol;
  }

  if (in_para)
  {
    str_puts(out, "<p>");
    str_append_str(out, &para);
    str_puts(out, "</p>\n");
  }
  str_free(&para);
}

// Code chunk
static void render_code_chunk(Str *out, const MDChunk *c)
{
  str_puts(out, "<pre><code");
  if (c->info_begin && c->info_end && c->info_begin < c->info_end)
  {
    str_puts(out, " title=\"");
    attr_escape(out, c->info_begin, (size_t)(c->info_end - c->info_begin));
    str_puts(out, "\"");
  }
  str_puts(out, ">");
  html_escape(out, c->begin, (size_t)(c->end - c->begin));
  str_puts(out, "</code></pre>\n");
}

// Public API
char* md_to_html(const char *markdown)
{
  if (!markdown) return NULL;

  char *norm = md_normalize_newlines(markdown);
  if (!norm) return NULL;

  MDChunks cs;
  cs.v = NULL;
  cs.n = 0;
  cs.cap = 0;
  md_split_fences(norm, &cs);

  Str out;
  str_init(&out);

  size_t i = 0;
  while (i < cs.n)
  {
    MDChunk *c = &cs.v[i];
    if (c->kind == MD_CHUNK_CODE)
    {
      size_t j = i;
      const char *info0 = c->info_begin;
      const char *info1 = c->info_end;

      str_puts(&out, "<pre><code");
      if (info0 && info1 && info0 < info1)
      {
        str_puts(&out, " title=\"");
        attr_escape(&out, info0, (size_t)(info1 - info0));
        str_puts(&out, "\"");
      }
      str_puts(&out, ">");

      bool first = true;
      while (j < cs.n && cs.v[j].kind == MD_CHUNK_CODE)
      {
        if (!first) str_putc(&out, '\n');
        html_escape(&out, cs.v[j].begin, (size_t)(cs.v[j].end - cs.v[j].begin));
        first = false;
        j += 1;
      }

      str_puts(&out, "</code></pre>\n");
      i = j;
      continue;
    }
    else
    {
      render_text_block(&out, c->begin, c->end);
      i += 1;
    }
  }

  free(cs.v);
  free(norm);

  char* built = str_data(&out);
  if (!built)
  {
    str_free(&out);
    return NULL;
  }
  size_t out_len = str_length(&out);
  char* result = (char*)malloc(out_len + 1);
  if (!result)
  {
    str_free(&out);
    return NULL;
  }
  memcpy(result, built, out_len + 1);
  str_free(&out);
  return result;
}

#endif // MD_IMPL
