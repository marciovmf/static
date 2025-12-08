/*
 * STDX - Minimal INI parser
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation, define X_IMPL_INI
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * X_INI_ALLOC / X_INI_REALLOC / X_INI_FREE before including.
 *
 * Notes:
 *   One flat string pool owns all normalized C strings (section names, keys, values).
 *   Sections and entries are indexed by arrays for direct and iterative access.
 */

#ifndef X_INI_H
#define X_INI_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef X_INI_API
#define X_INI_API
#endif

#define X_INI_VERSION_MAJOR 1
#define X_INI_VERSION_MINOR 1
#define X_INI_VERSION_PATCH 0
#define X_INI_VERSION (X_INI_VERSION_MAJOR*10000 + X_INI_VERSION_MINOR*100 + X_INI_VERSION_PATCH)

/* Tuning */
#define X_INI_DEFAULT_SECTIONS_CAP  16
#define X_INI_DEFAULT_ENTRIES_CAP   64
#define X_INI_MAX_LINE              4096

#ifdef __cplusplus
extern "C" {
#endif

typedef enum XIniErrorCode
{
  XINI_OK = 0,
  XINI_ERR_IO,
  XINI_ERR_MEMORY,
  XINI_ERR_SYNTAX,
  XINI_ERR_EXPECT_EQUALS,
  XINI_ERR_EXPECT_RBRACKET,
  XINI_ERR_UNTERMINATED_STRING
} XIniErrorCode;

typedef struct XIniError
{
  XIniErrorCode code;  /* error kind */
  int line;            /* 1-based line number where error was detected */
  int column;          /* 1-based column of the offending character/token */
  const char *message; /* short, static message string */
} XIniError;

typedef struct XIniSection
{
  const char *name;   /* section name or "" for the global section */
  int first_entry;    /* index of first entry for this section, or -1 if none */
} XIniSection;

typedef struct XIniEntry
{
  int section;        /* section index this entry belongs to */
  const char *key;    /* normalized key */
  const char *value;  /* normalized value (may be empty string) */
} XIniEntry;

typedef struct XIni
{
  /* string pool */
  char  *pool;
  size_t pool_size;
  size_t pool_cap;
  /* sections and entries */
  XIniSection *sections;
  int          sections_count;
  int          sections_cap;
  XIniEntry   *entries;
  int          entries_count;
  int          entries_cap;
  /* bookkeeping */
  int          global_section; // index of the synthetic global section ("")
} XIni;

/* Loading / lifetime */
X_INI_API bool        x_ini_load_file(const char *path, XIni *out_ini, XIniError *err);
/* data is copied/normalized; err may be NULL */
X_INI_API bool        x_ini_load_mem(const void *data, size_t size, XIni *out_ini, XIniError *err);
X_INI_API void        x_ini_free(XIni *ini);

/* Error string (static, thread-safe) */
X_INI_API const char* x_ini_err_str(XIniErrorCode code);

/* Queries */
X_INI_API const char* x_ini_get(const XIni *ini, const char *section, const char *key, const char *def_value);
X_INI_API int32_t     x_ini_get_i32(const XIni *ini, const char *section, const char *key, int32_t def_value);
X_INI_API float       x_ini_get_f32(const XIni *ini, const char *section, const char *key, float def_value);
X_INI_API bool        x_ini_get_bool(const XIni *ini, const char *section, const char *key, bool def_value);

/* Iteration helpers */
X_INI_API int         x_ini_section_count(const XIni *ini);
X_INI_API const char* x_ini_section_name(const XIni *ini, int section_index);
X_INI_API int         x_ini_key_count(const XIni *ini, int section_index);
X_INI_API const char* x_ini_key_name(const XIni *ini, int section_index, int key_index);
X_INI_API const char* x_ini_value_at(const XIni *ini, int section_index, int key_index);

#ifdef __cplusplus
} // extern "C"
#endif


#ifdef X_IMPL_INI

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Allocator override */
#ifndef X_INI_ALLOC
#define X_INI_ALLOC(sz)        malloc(sz)
#define X_INI_REALLOC(p,sz)    realloc((p),(sz))
#define X_INI_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- internal helpers ---------------- */

static void s_x_set_err(XIniError *err, XIniErrorCode code, int line, int column, const char *msg)
{
  if (err)
  {
    err->code = code;
    err->line = line;
    err->column = column;
    err->message = msg;
  }
}

static const char *s_x_err_msg(XIniErrorCode code)
{
  switch (code)
  {
    case XINI_OK:                  return "ok";
    case XINI_ERR_IO:              return "i/o error";
    case XINI_ERR_MEMORY:          return "out of memory";
    case XINI_ERR_SYNTAX:          return "syntax error";
    case XINI_ERR_EXPECT_EQUALS:   return "expected '='";
    case XINI_ERR_EXPECT_RBRACKET: return "expected ']'";
    case XINI_ERR_UNTERMINATED_STRING: return "unterminated string";
    default:                       return "unknown error";
  }
}

static void *s_x_realloc_grow(void *ptr, size_t elem_size, int *cap)
{
  int new_cap = (*cap > 0) ? (*cap * 2) : 8;
  size_t bytes = (size_t)new_cap * elem_size;
  void *np = X_INI_REALLOC(ptr, bytes);
  if (!np) return NULL;
  *cap = new_cap;
  return np;
}

static bool s_x_pool_init(XIni *ini, size_t cap)
{
  ini->pool = (char *)X_INI_ALLOC(cap);
  if (!ini->pool) return false;
  ini->pool_size = 0;
  ini->pool_cap = cap;
  return true;
}

static const char *s_x_pool_add(XIni *ini, const char *src, size_t len)
{
  if (ini->pool_size + len + 1 > ini->pool_cap)
  {
    size_t ncap = ini->pool_cap ? ini->pool_cap * 2 : 1024;
    while (ini->pool_size + len + 1 > ncap) ncap *= 2;
    char *np = (char *)X_INI_REALLOC(ini->pool, ncap);
    if (!np) return NULL;
    ini->pool = np;
    ini->pool_cap = ncap;
  }
  char *dst = ini->pool + ini->pool_size;
  memcpy(dst, src, len);
  dst[len] = '\0';
  ini->pool_size += len + 1;
  return dst;
}

static const char *s_x_pool_add_cstr(XIni *ini, const char *s)
{
  return s_x_pool_add(ini, s, strlen(s));
}

static char *s_x_trim_inplace(char *s)
/* trims leading/trailing ASCII whitespace; returns pointer to first non-space */
{
  if (!s) return s;
  char *start = s;
  while (*start && isspace((unsigned char)*start)) { ++start; }

  if (*start == '\0') return start;

  char *end = start + strlen(start) - 1;
  while (end >= start && isspace((unsigned char)*end)) { *end-- = '\0'; }
  return start;
}

/* remove inline comments ';' or '#' found outside quotes */
static void s_x_chomp_inline_comment_unquoted(char *s)
{
  bool in_quote = false;
  for (char *p = s; *p; ++p)
  {
    if (*p == '\"')
    {
      /* toggle only if not escaped */
      bool escaped = (p > s && p[-1] == '\\');
      if (!escaped) in_quote = !in_quote;
    }
    if (!in_quote && (*p == ';' || *p == '#'))
    {
      *p = '\0';
      break;
    }
  }
}

static int s_x_find_section(const XIni *ini, const char *section)
{
  for (int i = 0; i < ini->sections_count; ++i)
  {
    if (strcmp(ini->sections[i].name, section) == 0) return i;
  }
  return -1;
}

static int s_x_count_keys_in_section(const XIni *ini, int section_index)
{
  int c = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    if (ini->entries[i].section == section_index) ++c;
  }
  return c;
}

static int s_x_stricmp(const char *a, const char *b)
{
  unsigned char ca, cb;
  while (*a && *b)
  {
    ca = (unsigned char)tolower((unsigned char)*a);
    cb = (unsigned char)tolower((unsigned char)*b);
    if (ca != cb) return (int)ca - (int)cb;
    ++a; ++b;
  }
  return (int)(unsigned char)tolower((unsigned char)*a)
       - (int)(unsigned char)tolower((unsigned char)*b);
}

/* decode a quoted string: handles \t \n \\ \" ; returns pointer in pool (no quotes) */
static const char *s_x_decode_quoted_into_pool(XIni *ini, const char *start_quote,
                                               int *out_error_col, bool *ok)
{
  *ok = false;
  const char *s = start_quote;
  if (*s != '\"') return NULL;
  ++s; /* skip opening quote */

  /* temporary dynamic buffer */
  size_t cap = 64, len = 0;
  char *tmp = (char *)X_INI_ALLOC(cap);
  if (!tmp) return NULL;

  int col = 1; /* relative column inside the quoted payload */

  while (*s)
  {
    char c = *s++;
    if (c == '\"')
    {
      /* end of string (unescaped quote) */
      const char *pooled = s_x_pool_add(ini, tmp, len);
      X_INI_FREE(tmp);
      *ok = (pooled != NULL);
      return pooled;
    }
    if (c == '\\')
    {
      char n = *s++;
      if (n == '\0') { *out_error_col = col; X_INI_FREE(tmp); return NULL; }
      switch (n)
      {
        case 'n':  c = '\n'; break;
        case 't':  c = '\t'; break;
        case '\\': c = '\\'; break;
        case '\"': c = '\"'; break;
        default:   c = n;    break; /* pass-through unknown escapes */
      }
      /* fallthrough with translated c */
    }
    /* push c */
    if (len + 1 >= cap)
    {
      size_t ncap = cap * 2;
      char *np = (char *)X_INI_REALLOC(tmp, ncap);
      if (!np) { X_INI_FREE(tmp); return NULL; }
      tmp = np; cap = ncap;
    }
    tmp[len++] = c;
    ++col;
  }

  /* if we got here, string was unterminated */
  *out_error_col = col;
  X_INI_FREE(tmp);
  return NULL;
}

/* ---------------- public implementation ---------------- */

X_INI_API const char* x_ini_err_str(XIniErrorCode code)
{
  return s_x_err_msg(code);
}

X_INI_API bool x_ini_load_mem(const void *data, size_t size, XIni *out_ini, XIniError *err)
{
  if (err) { err->code = XINI_OK; err->line = 0; err->column = 0; err->message = s_x_err_msg(XINI_OK); }
  if (!out_ini) { s_x_set_err(err, XINI_ERR_SYNTAX, 0, 0, "null output ini"); return false; }

  memset(out_ini, 0, sizeof(*out_ini));
  if (!s_x_pool_init(out_ini, size > 0 ? (size * 2) : 1024))
  {
    s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY));
    return false;
  }

  out_ini->sections = (XIniSection *)X_INI_ALLOC(sizeof(XIniSection) * X_INI_DEFAULT_SECTIONS_CAP);
  out_ini->entries  = (XIniEntry   *)X_INI_ALLOC(sizeof(XIniEntry)   * X_INI_DEFAULT_ENTRIES_CAP);
  if (!out_ini->sections || !out_ini->entries)
  {
    s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY));
    x_ini_free(out_ini);
    return false;
  }
  out_ini->sections_cap = X_INI_DEFAULT_SECTIONS_CAP;
  out_ini->entries_cap  = X_INI_DEFAULT_ENTRIES_CAP;

  /* global section ("") */
  const char *gs = s_x_pool_add_cstr(out_ini, "");
  if (!gs) { s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); x_ini_free(out_ini); return false; }
  out_ini->sections[0].name = gs;
  out_ini->sections[0].first_entry = -1;
  out_ini->sections_count = 1;
  out_ini->global_section = 0;

  /* copy data to a working buffer so we can slice lines easily */
  char *buf = (char *)X_INI_ALLOC(size + 1);
  if (!buf) { s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); x_ini_free(out_ini); return false; }
  memcpy(buf, data, size);
  buf[size] = '\0';

  int current_section = out_ini->global_section;

  /* parse line by line */
  char *cursor = buf;
  int line_no = 0;

  while (*cursor)
  {
    char *line = cursor;
    /* advance to next line */
    while (*cursor && *cursor != '\n' && *cursor != '\r') { ++cursor; }
    if (*cursor == '\r') { *cursor++ = '\0'; if (*cursor == '\n') { ++cursor; } }
    else if (*cursor == '\n') { *cursor++ = '\0'; }
    ++line_no;

    s_x_chomp_inline_comment_unquoted(line);
    char *raw = line;
    char *trim = s_x_trim_inplace(raw);
    if (*trim == '\0') continue; /* empty */
    if (*trim == ';' || *trim == '#') continue; /* full-line comment */

    if (*trim == '[')
    {
      /* section line: [name] */
      char *rbr = strchr(trim, ']');
      if (!rbr)
      {
        s_x_set_err(err, XINI_ERR_EXPECT_RBRACKET, line_no, (int)(strlen(trim) + 1), s_x_err_msg(XINI_ERR_EXPECT_RBRACKET));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
      *rbr = '\0';
      char *name = s_x_trim_inplace(trim + 1);
      name = s_x_trim_inplace(name);
      const char *sname = s_x_pool_add_cstr(out_ini, name);
      if (!sname)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }

      if (out_ini->sections_count == out_ini->sections_cap)
      {
        void *np = s_x_realloc_grow(out_ini->sections, sizeof(XIniSection),
            &out_ini->sections_cap);
        if (!np)
        {
          s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
          X_INI_FREE(buf);
          x_ini_free(out_ini);
          return false;
        }
        out_ini->sections = (XIniSection *)np;
      }
      current_section = out_ini->sections_count;
      out_ini->sections[current_section].name = sname;
      out_ini->sections[current_section].first_entry = -1;
      out_ini->sections_count += 1;
      continue;
    }

    /* key = value */
    char *eq = strchr(trim, '=');
    if (!eq)
    {
      /* report column at first non-space char */
      int col = (int)(trim - raw) + 1;
      s_x_set_err(err, XINI_ERR_EXPECT_EQUALS, line_no, col, s_x_err_msg(XINI_ERR_EXPECT_EQUALS));
      X_INI_FREE(buf);
      x_ini_free(out_ini);
      return false;
    }
    *eq = '\0';
    char *k = s_x_trim_inplace(trim);
    char *v = s_x_trim_inplace(eq + 1);

    /* value normalization:
       - if quoted: decode escapes and drop quotes
       - else: chomp inline comments, then trim again
    */
    const char *vv = NULL;
    if (*v == '\"')
    {
      int err_col = 1;
      bool ok = false;
      vv = s_x_decode_quoted_into_pool(out_ini, v, &err_col, &ok);
      if (!ok || !vv)
      {
        s_x_set_err(err, XINI_ERR_UNTERMINATED_STRING, line_no, (int)(v - raw) + err_col + 1, s_x_err_msg(XINI_ERR_UNTERMINATED_STRING));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
    }
    else
    {
      s_x_chomp_inline_comment_unquoted(v);
      v = s_x_trim_inplace(v);
      vv = s_x_pool_add_cstr(out_ini, v);
      if (!vv)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
    }

    const char *kk = s_x_pool_add_cstr(out_ini, k);
    if (!kk)
    {
      s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
      X_INI_FREE(buf);
      x_ini_free(out_ini);
      return false;
    }

    if (out_ini->entries_count == out_ini->entries_cap)
    {
      void *np = s_x_realloc_grow(out_ini->entries, sizeof(XIniEntry),
                                  &out_ini->entries_cap);
      if (!np)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
      out_ini->entries = (XIniEntry *)np;
    }

    int idx = out_ini->entries_count++;
    out_ini->entries[idx].section = current_section;
    out_ini->entries[idx].key = kk;
    out_ini->entries[idx].value = vv;

    if (out_ini->sections[current_section].first_entry < 0)
    {
      out_ini->sections[current_section].first_entry = idx;
    }
  }

  X_INI_FREE(buf);
  return true;
}

X_INI_API bool x_ini_load_file(const char *path, XIni *out_ini, XIniError *err)
{
  if (err) { err->code = XINI_OK; err->line = 0; err->column = 0; err->message = s_x_err_msg(XINI_OK); }
  if (!path || !out_ini) { s_x_set_err(err, XINI_ERR_SYNTAX, 0, 0, "null argument"); return false; }

  FILE *f = fopen(path, "rb");
  if (!f) { s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }
  long len = ftell(f);
  if (len < 0) { fclose(f); s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }
  rewind(f);

  char *buf = (char *)X_INI_ALLOC((size_t)len + 1);
  if (!buf) { fclose(f); s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); return false; }

  size_t rd = fread(buf, 1, (size_t)len, f);
  fclose(f);
  if (rd != (size_t)len)
  {
    X_INI_FREE(buf);
    s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO));
    return false;
  }
  buf[len] = '\0';

  bool ok = x_ini_load_mem(buf, (size_t)len, out_ini, err);
  X_INI_FREE(buf);
  return ok;
}

X_INI_API void x_ini_free(XIni *ini)
{
  if (!ini) return;

  X_INI_FREE(ini->pool);
  X_INI_FREE(ini->sections);
  X_INI_FREE(ini->entries);

  memset(ini, 0, sizeof(*ini));
  ini->global_section = -1;
}

X_INI_API const char *x_ini_get(const XIni *ini, const char *section, const char *key, const char *def_value)
{
  if (!ini || !key) return def_value;

  const char *secname = section ? section : "";
  int sidx = s_x_find_section(ini, secname);
  if (sidx < 0) return def_value;

  /* last definition wins -> iterate from the end */
  for (int i = ini->entries_count - 1; i >= 0; --i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == sidx && strcmp(e->key, key) == 0) return e->value;
  }
  return def_value;
}

X_INI_API int32_t x_ini_get_i32(const XIni *ini, const char *section, const char *key, int32_t def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;
  char *end = NULL;
  long v = strtol(s, &end, 0);
  if (end == s) return def_value;
  return (int32_t)v;
}

X_INI_API float x_ini_get_f32(const XIni *ini, const char *section, const char *key, float def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;
  char *end = NULL;
  float v = (float)strtod(s, &end);
  if (end == s) return def_value;
  return v;
}

X_INI_API bool x_ini_get_bool(const XIni *ini, const char *section, const char *key, bool def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;

  /* case-insensitive checks for common forms */
  char c0 = (char)tolower((unsigned char)s[0]);
  if ((c0 == 't' && s_x_stricmp(s, "true")  == 0) ||
      (c0 == 'y' && s_x_stricmp(s, "yes")   == 0) ||
      (c0 == 'o' && s_x_stricmp(s, "on")    == 0) ||
      (c0 == '1' && s[1] == '\0'))
  {
    return true;
  }
  if ((c0 == 'f' && s_x_stricmp(s, "false") == 0) ||
      (c0 == 'n' && s_x_stricmp(s, "no")    == 0) ||
      (c0 == 'o' && s_x_stricmp(s, "off")   == 0) ||
      (c0 == '0' && s[1] == '\0'))
  {
    return false;
  }
  return def_value;
}

X_INI_API int x_ini_section_count(const XIni *ini)
{
  if (!ini) return 0;
  return ini->sections_count;
}

X_INI_API const char *x_ini_section_name(const XIni *ini, int section_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;
  return ini->sections[section_index].name;
}

X_INI_API int x_ini_key_count(const XIni *ini, int section_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return 0;
  return s_x_count_keys_in_section(ini, section_index);
}

X_INI_API const char *x_ini_key_name(const XIni *ini, int section_index, int key_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;

  /* linear scan; stable order of appearance */
  int seen = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == section_index)
    {
      if (seen == key_index) return e->key;
      ++seen;
    }
  }
  return NULL;
}

X_INI_API const char *x_ini_value_at(const XIni *ini, int section_index, int key_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;

  int seen = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == section_index)
    {
      if (seen == key_index) return e->value;
      ++seen;
    }
  }
  return NULL;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // X_IMPL_INI
#endif // X_INI_H
