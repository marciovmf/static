/*
 * STDX - Dynamic String Builder for C
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_STRBUILDER
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * X_STRBUILDER_ALLOC / X_STRBUILDER_REALLOC / X_STRBUILDER_FREE before including.
 *
 * Notes:
 * This header provides a simple interface for constructing strings efficiently:
 *   - Dynamic growth as data is appended
 *   - Append strings, characters, substrings, and formatted text
 *   - Convert to null-terminated C string
 *   - Clear or destroy the builder when done
 */

#ifndef X_STRBUILDER_H
#define X_STRBUILDER_H

#ifndef X_STRBUILDER_API
#define X_STRBUILDER_API
#endif

#define X_STRBUILDER_VERSION_MAJOR 1
#define X_STRBUILDER_VERSION_MINOR 0
#define X_STRBUILDER_VERSION_PATCH 0
#define X_STRBUILDER_VERSION (X_STRBUILDER_VERSION_MAJOR * 10000 + X_STRBUILDER_VERSION_MINOR * 100 + X_STRBUILDER_VERSION_PATCH)

#ifndef STRBUILDER_STACK_BUFFER_SIZE
#define STRBUILDER_STACK_BUFFER_SIZE 255
#endif

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XStrBuilder XStrBuilder;
  typedef struct XWStrBuilder XWStrBuilder;

#define x_strbuilder_append_cstr(sb, str) x_strbuilder_append((sb), (str))

  /**
   * @brief Creates a XStrBuilder
   * @param (none)
   * @return pointer to the XStrBuilder or NULL if creation fails
   */
  XStrBuilder*  x_strbuilder_create();

  /**
   * @brief Appends a C-string to the builder
   * @param sb Destination builder
   *  @param str Source null-terminated string (NULL is ignored)
   */
  void   x_strbuilder_append(XStrBuilder *sb, const char *str);

  /**
   * @brief Appends one character
   * @param sb Destination builder
   * @param c Character to append
   */
  void   x_strbuilder_append_char(XStrBuilder* sb, char c);

  /**
   * @brief Appends formatted text (printf-style)
   * @param sb Destination builder
   * @param format Format string (printf-style)
   */
  void   x_strbuilder_append_format(XStrBuilder *sb, const char *format, ...);

  /**
   * @brief Appends a substring
   * @param sb Destination builder
   * @param start Pointer to first byte
   * @param length Number of bytes to copy
   */
  void   x_strbuilder_append_substring(XStrBuilder *sb, const char *start, size_t length);

  /**
   * @brief Returns internal buffer pointer
   * @param sb Builder to query
   * @return Pointer to internal NUL-terminated buffer (invalidated by later appends)
   */
  char*  x_strbuilder_to_string(const XStrBuilder *sb);

  /**
   * @brief Destroys the builder and frees memory
   * @param sb Builder to destroy (may be NULL)
   */
  void   x_strbuilder_destroy(XStrBuilder *sb);

  /**
   * @brief Clears the builder content
   * @param sb Builder to clear
   */
  void   x_strbuilder_clear(XStrBuilder *sb);

  /**
   * @brief Returns current length in bytes
   * @param sb Builder to query
   * @return Length in bytes (excluding the NUL)
   */
  size_t x_strbuilder_length(XStrBuilder *sb);

  /**
   * @brief Appends a UTF-8 substring by code-point range
   * @param sb Destination builder
   * @param utf8 Source UTF-8 string
   * @param start_cp Start code-point index
   * @param len_cp Number of code points to append
   */
  void   x_strbuilder_append_utf8_substring(XStrBuilder* sb, const char* utf8, size_t start_cp, size_t len_cp); // UTF-8-aware append
  
  /**
   * @brief Counts UTF-8 code points contained in the builder
   * @param sb Builder to query
   * @return Number of Unicode code points
   */
  size_t x_strbuilder_utf8_charlen(const XStrBuilder* sb); // UTF-8 aware length

  /**
   * @brief Creates a wide-character string builder
   * param (none)
   * @return Pointer to builder or NULL on failure
   */
  XWStrBuilder* x_wstrbuilder_create();

  /**
   * @brief Appends a wide C-string
   * @param sb Destination builder
   * @param str Source wide string (NULL is ignored)
   */
  void   x_wstrbuilder_append(XWStrBuilder* sb, const wchar_t* str);

  /**
   * @brief Appends one wide character
   * @param sb Destination builder
   * @param c Character to append
   */
  void   x_wstrbuilder_append_char(XWStrBuilder* sb, wchar_t c);

  /**
   * @brief Appends formatted wide text (printf-style)
   * @param sb Destination builder
   * @param format Wide format string
   */
  void   x_wstrbuilder_append_format(XWStrBuilder* sb, const wchar_t* format, ...);

  /**
   * @brief Clears the wide builder content
   * @param sb Builder to clear
   */
  void   x_wstrbuilder_clear(XWStrBuilder* sb);

  /**
   * @brief Destroys the wide builder and frees memory
   * @param sb Builder to destroy (may be NULL)
   */
  void   x_wstrbuilder_destroy(XWStrBuilder* sb);

  /**
   * @brief Returns internal wide buffer pointer
   *  param sb Builder to query
   *  return Pointer to internal NUL-terminated wide buffer (invalidated by later appends)
   */
  wchar_t* x_wstrbuilder_to_string(const XWStrBuilder* sb);

  /**
   * @brief Returns current length in wide characters
   *  param sb Builder to query
   *  return Number of wchar_t (excluding the NUL)
   */
  size_t x_wstrbuilder_length(const XWStrBuilder* sb);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_STRBUILDER

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#ifndef X_STRBUILDER_ALLOC
#define X_STRBUILDER_ALLOC(sz)        malloc(sz)
#define X_STRBUILDER_REALLOC(p,sz)    realloc((p),(sz))
#define X_STRBUILDER_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  struct XStrBuilder
  {
    char *data;
    size_t capacity;
    size_t length;
  };

  struct XWStrBuilder
  {
    wchar_t *data;
    size_t capacity;
    size_t length;
  };

  static int s_strbuilder_reserve(XStrBuilder* sb, size_t min_cap)
  {
    if (!sb) return 0;
    if (sb->capacity >= min_cap) return 1;
    size_t cap = sb->capacity ? sb->capacity : 16u;
    while (cap < min_cap) {
      if (cap > (SIZE_MAX/2u)) { cap = min_cap; break; }
      cap *= 2u;
    }
    char* p = (char*)X_STRBUILDER_REALLOC(sb->data, cap * sizeof(char));
    if (!p) return 0;
    sb->data = p;
    sb->capacity = cap;
    return 1;
  }

  static int s_wstrbuilder_reserve(XWStrBuilder* sb, size_t min_cap)
  {
    if (!sb) return 0;
    if (sb->capacity >= min_cap) return 1;
    size_t cap = sb->capacity ? sb->capacity : 16u;
    while (cap < min_cap) {
      if (cap > (SIZE_MAX/2u)) { cap = min_cap; break; }
      cap *= 2u;
    }
    wchar_t* p = (wchar_t*)X_STRBUILDER_REALLOC(sb->data, cap * sizeof(wchar_t));
    if (!p) return 0;
    sb->data = p;
    sb->capacity = cap;
    return 1;
  }

  static size_t s_utf8_advance_index(const char* s, size_t maxlen, size_t count)
  {
    size_t i = 0, cp = 0;
    while (i < maxlen && cp < count)
    {
      unsigned char c = (unsigned char)s[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else break;
      cp++;
    }
    return i;
  }

  X_STRBUILDER_API XStrBuilder* x_strbuilder_create()
  {
    XStrBuilder* sb = (XStrBuilder*)  X_STRBUILDER_ALLOC(sizeof(XStrBuilder));
    if (!sb) return NULL;
    sb->capacity = 16; // Initial capacity
    sb->data = (char *) X_STRBUILDER_ALLOC(sb->capacity * sizeof(char));
    if (!sb->data) { X_STRBUILDER_FREE(sb); return NULL; }
    sb->length = 0;
    sb->data[0] = '\0'; // Null-terminate the string
    return sb;
  }

  X_STRBUILDER_API void x_strbuilder_append(XStrBuilder *sb, const char *str)
  {
    size_t strLen = strlen(str);
    size_t newLength = sb->length + strLen;
    if (!s_strbuilder_reserve(sb, newLength + 1)) return;
    memcpy(sb->data + sb->length, str, strLen);
    sb->length = newLength;
    sb->data[sb->length] = '\0';
  }

  X_STRBUILDER_API void x_strbuilder_append_format(XStrBuilder* sb, const char* fmt, ...)
  {
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);

    int need = vsnprintf(NULL, 0, fmt, args);   // measure
    va_end(args);
    if (need < 0) { va_end(args2); return; }

    size_t uneed = (size_t)need;
    if (!s_strbuilder_reserve(sb, sb->length + uneed + 1)) { va_end(args2); return; }

    // write at the end
    vsnprintf(sb->data + sb->length, uneed + 1, fmt, args2);
    va_end(args2);
    sb->length += uneed;
  }

  X_STRBUILDER_API void x_strbuilder_append_char(XStrBuilder* sb, char c)
  {
    char s[2] =
    {c, 0};
    x_strbuilder_append(sb, s);
  }

  X_STRBUILDER_API void x_strbuilder_append_substring(XStrBuilder *sb, const char *start, size_t length)
  {
    size_t newLength = sb->length + length;
    if (!s_strbuilder_reserve(sb, newLength + 1)) return;
    memcpy(sb->data + sb->length, start, length);
    sb->length = newLength;
    sb->data[sb->length] = '\0';
  }

  X_STRBUILDER_API char* x_strbuilder_to_string(const XStrBuilder *sb)
  {
    return sb->data;
  }

  X_STRBUILDER_API void x_strbuilder_destroy(XStrBuilder *sb)
  {
    X_STRBUILDER_FREE(sb->data);
    sb->data = NULL;
    sb->capacity = 0;
    sb->length = 0;
    X_STRBUILDER_FREE(sb);
  }

  X_STRBUILDER_API void x_strbuilder_clear(XStrBuilder *sb)
  {
    if (sb->length > 0)
    {
      sb->data[0] = 0;
      sb->length = 0;
    }
  }

  X_STRBUILDER_API size_t x_strbuilder_length(XStrBuilder *sb)
  {
    return sb->length;
  }

  X_STRBUILDER_API void x_strbuilder_append_utf8_substring(XStrBuilder* sb, const char* utf8, size_t start_cp, size_t len_cp)
  {
    size_t byte_start = s_utf8_advance_index(utf8, strlen(utf8), start_cp);
    size_t byte_len = s_utf8_advance_index(utf8 + byte_start, strlen(utf8 + byte_start), len_cp);
    x_strbuilder_append_substring(sb, utf8 + byte_start, byte_len);
  }

  X_STRBUILDER_API size_t x_strbuilder_utf8_charlen(const XStrBuilder* sb)
  {
    size_t count = 0;
    const char* s = sb->data;
    while (*s)
    {
      unsigned char c = (unsigned char)*s;
      if ((c & 0x80) == 0x00) s += 1;
      else if ((c & 0xE0) == 0xC0) s += 2;
      else if ((c & 0xF0) == 0xE0) s += 3;
      else if ((c & 0xF8) == 0xF0) s += 4;
      else break;
      count++;
    }
    return count;
  }

  X_STRBUILDER_API XWStrBuilder* x_wstrbuilder_create()
  {
    XWStrBuilder* sb = (XWStrBuilder*)X_STRBUILDER_ALLOC(sizeof(XWStrBuilder));
    if (!sb) return NULL;
    sb->capacity = 16;
    sb->length = 0;
    sb->data = (wchar_t*)X_STRBUILDER_ALLOC(sb->capacity * sizeof(wchar_t));
    if (!sb->data) { X_STRBUILDER_FREE(sb); return NULL; }
    sb->data[0] = L'\0';
    return sb;
  }

  X_STRBUILDER_API void x_wstrbuilder_grow(XWStrBuilder* sb, size_t needed_len)
  {
    if (sb->length + needed_len + 1 > sb->capacity)
    {
      size_t want = sb->capacity;
      while (sb->length + needed_len + 1 > want)
      {
        if (want > (SIZE_MAX/2)) { want = sb->length + needed_len + 1; break; }
        want *= 2;
      }
      wchar_t* p = (wchar_t*)X_STRBUILDER_REALLOC(sb->data, want * sizeof(wchar_t));
      if (!p) return;
      sb->data = p;
      sb->capacity = want;
    }
  }

  X_STRBUILDER_API void x_wstrbuilder_append(XWStrBuilder* sb, const wchar_t* str)
  {
    size_t len = wcslen(str);
    x_wstrbuilder_grow(sb, len);
    memcpy(sb->data + sb->length, str, len * sizeof(wchar_t));
    sb->length += len;
    sb->data[sb->length] = L'\0';
  }

  X_STRBUILDER_API void x_wstrbuilder_append_char(XWStrBuilder* sb, wchar_t c)
  {
    wchar_t buf[2] =
    {c, 0};
    x_wstrbuilder_append(sb, buf);
  }

  X_STRBUILDER_API void x_wstrbuilder_append_format(XWStrBuilder* sb, const wchar_t* format, ...)
  {
    va_list args, args2;
    va_start(args, format);
    va_copy(args2, args);
#if defined(_MSC_VER)
    int need = _vscwprintf(format, args);
    if (need < 0) { va_end(args); va_end(args2); return; }
    size_t uneed = (size_t)need;
    x_wstrbuilder_grow(sb, uneed);
    vswprintf(sb->data + sb->length, uneed + 1, format, args2);
    sb->length += uneed;
    sb->data[sb->length] = L'\0';
#else
    /* portable grow loop */
    size_t cap = 64;
    while (1) {
      x_wstrbuilder_grow(sb, cap);
      int written = vswprintf(sb->data + sb->length, cap + 1, format, args);
      if (written >= 0 && (size_t)written <= cap) {
        sb->length += (size_t)written;
        break;
      }
      cap *= 2;
    }
#endif
    va_end(args2);
    va_end(args);
  }

  X_STRBUILDER_API void x_wstrbuilder_clear(XWStrBuilder* sb)
  {
    sb->length = 0;
    sb->data[0] = L'\0';
  }

  X_STRBUILDER_API void x_wstrbuilder_destroy(XWStrBuilder* sb)
  {
    X_STRBUILDER_FREE(sb->data);
    X_STRBUILDER_FREE(sb);
  }

  X_STRBUILDER_API wchar_t* x_wstrbuilder_to_string(const XWStrBuilder* sb)
  {
    return sb->data;
  }

  X_STRBUILDER_API size_t x_wstrbuilder_length(const XWStrBuilder* sb)
  {
    return sb->length;
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_STRBUILDER
#endif // X_STRBUILDER_H

