/*
 * STDX - Lightweight String Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_STRING
 * in **one** source file before including this header.
 *
 * Notes:
 *  This header provides:
 *   - C string helpers: case-insensitive prefix/suffix matching
 *   - XSmallstr: fixed-capacity, stack-allocated strings
 *   - XSlice: immutable, non-owning views into C strings
 *   - Tokenization and trimming
 *   - UTF-8-aware string length calculation
 *   - Fast substring and search operations
 *   - Case-sensitive and case-insensitive comparisons
 */

#ifndef X_STRING_H
#define X_STRING_H

#include <stddef.h>   // size_t
#include <wchar.h>    // wchar_t used in public types
#include <stdint.h>   // uint32_t, int32_t
#include <stdbool.h>
#include <stdarg.h>

#ifndef X_STRING_API
#define X_STRING_API
#endif

#define X_STRING_VERSION_MAJOR 1
#define X_STRING_VERSION_MINOR 0
#define X_STRING_VERSION_PATCH 0
#define X_STRING_VERSION (X_STRING_VERSION_MAJOR * 10000 + X_STRING_VERSION_MINOR * 100 + X_STRING_VERSION_PATCH)

#ifndef X_SMALLSTR_MAX_LENGTH
#define X_SMALLSTR_MAX_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct 
  {
    const char* ptr;
    size_t length;
  } XSlice;

  typedef struct
  {
    const wchar_t* ptr;
    size_t length; // In wide characters
  } XWStrview;

  typedef struct 
  {
    char buf[X_SMALLSTR_MAX_LENGTH + 1];
    size_t length;
  } XSmallstr;

  typedef struct
  {
    wchar_t buf[X_SMALLSTR_MAX_LENGTH + 1];
    size_t length; // In wide characters
  } XWSmallstr;

  /* UTF-8 decode result codes (negative values are errors) */
  typedef enum XUtf8Error
  {
    X_UTF8_OK           = 0,  // placeholder: not returned by x_utf8_decode
    X_UTF8_ERR_EOF      = -1, // ptr == end (no bytes available)
    X_UTF8_ERR_INVALID  = -2, // invalid leading/trailing byte sequence
    X_UTF8_ERR_OVERLONG = -3, // overlong form detected
    X_UTF8_ERR_RANGE    = -4  // codepoint > 0x10FFFF or surrogate
  } XUtf8Error;

  /**
   * @brief Sets the process locale used by locale-aware string operations.
   * @param locale Parameter.
   * @return No return value.
   */
  X_STRING_API void      x_set_locale(const char* locale);

  // -------------------------------------------------------------------------------------
  // C-String functions
  // -------------------------------------------------------------------------------------

  /**
   * @brief Computes a 32-bit FNV hash for a null-terminated C-string.
   * @param str Input C-string.
   * @return 32 bit FNV hash
   */
  X_STRING_API uint32_t  x_cstr_hash(const char* str);

  /**
   * @brief Searches for a substring within a null-terminated C-string.
   * @param haystack Input string to search in.
   * @param needle Substring to search for.
   * @return pointer within haystack where needle is found
   */
  X_STRING_API char*     x_cstr_str(const char *haystack, const char *needle);

  /**
   * @brief Checks whether the string ends with the specified suffix.
   * @param str Input C-string.
   * @param suffix Suffix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_cstr_ends_with(const char* str, const char* suffix);

  /**
   * @brief Checks whether a null-terminated C-string starts with the given prefix.
   * @param str Input C-string.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_cstr_starts_with(const char* str, const char* prefix);

  /**
   * @brief Case-insensitive check whether the string ends with the specified suffix.
   * @param str Input C-string.
   * @param suffix Suffix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_cstr_ends_with_ci(const char* str, const char* suffix);

  /**
   * @brief Checks whether a null-terminated C-string starts with the given prefix.
   * @param str Input C-string.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_cstr_starts_with_ci(const char* str, const char* prefix);

  /**
   * @brief Checks whether the string starts with the specified prefix.
   * @param str Input C-string.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix);

  /**
   * @brief Checks whether a null-terminated wide C-string ends with the given suffix.
   * @param str Input C-string.
   * @param suffix Suffix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix);

  /**
   * @brief Case-insensitive check whether the string starts with the specified prefix.
   * @param str Input C-string.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_wcstr_starts_with_ci(const wchar_t* str, const wchar_t* prefix);

  /**
   * @brief Checks whether a null-terminated wide C-string ends with the given suffix.
   * @param str Input C-string.
   * @param suffix Suffix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_wcstr_ends_with_ci(const wchar_t* str, const wchar_t* suffix);

  /**
   * @brief Case-insensitively compares two wide C-strings.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wcstr_cicmp(const wchar_t* a, const wchar_t* b);

  // UTF-8 and wide string conversions and utility functions
  /**
   * @brief Utility for null-terminated C-strings.
   * @param src Source buffer or string.
   * @param dest Destination buffer.
   * @param max Capacity of the destination buffer (in elements).
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_cstr_to_wcstr(const char* src, wchar_t* dest, size_t max);

  // -------------------------------------------------------------------------------------
  // Encoding conversions
  // -------------------------------------------------------------------------------------

  /**
   * @brief Converts between string encodings.
   * @param wide Destination wide-character buffer.
   * @param utf8 Source UTF-8 string.
   * @param max Capacity of the destination buffer (in elements).
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_wcstr_to_utf8(const wchar_t* wide, char* utf8, size_t max);

  /**
   * @brief Utility for null-terminated wide C-strings.
   * @param src Source buffer or string.
   * @param dest Destination buffer.
   * @param max Capacity of the destination buffer (in elements).
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_wcstr_to_cstr(const wchar_t* src, char* dest, size_t max);

  /**
   * @brief Converts between string encodings.
   * @param utf8 Source UTF-8 string.
   * @param wide Destination wide-character buffer.
   * @param max Capacity of the destination buffer (in elements).
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_utf8_to_wcstr(const char* utf8, wchar_t* wide, size_t max);

  // -------------------------------------------------------------------------------------
  // UTF8 functions
  // -------------------------------------------------------------------------------------

  /**
   * @brief Returns the number of UTF-8 codepoints in a null-terminated string.
   * @param utf8 Source UTF-8 string.
   * @return Number of UTF-8 codepoints.
   */
  X_STRING_API size_t    x_utf8_strlen(const char* utf8);

  /**
   * @brief Compares two null-terminated UTF-8 strings.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_utf8_strcmp(const char* a, const char* b);

  /**
   * @brief Converts ASCII letters to lower case; leaves multibyte UTF-8 unchanged.
   * @param dest Destination buffer.
   * @param src Source buffer or string.
   * @return Function-specific result.
   */
  X_STRING_API char*     x_utf8_tolower(char* dest, const char* src);

  /**
   * @brief Converts ASCII letters to upper case; leaves multibyte UTF-8 unchanged.
   * @param dest Destination buffer.
   * @param src Source buffer or string.
   * @return Function-specific result.
   */
  X_STRING_API char*     x_utf8_toupper(char* dest, const char* src);

  /**
   * @brief Checks whether the string contains exactly one UTF-8 codepoint.
   * @param s Destination small string.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_utf8_is_single_char(const char* s);

  /**
   * @brief Decodes a single UTF-8 codepoint from the specified byte span.
   * @param ptr Current read pointer within the byte span.
   * @param end One-past-the-end pointer for the byte span.
   * @param out_len Output: number of codepoints consumed on success or bytes to skip on error.
   * @return Decoded codepoint on success; negative X_UTF8_ERR_* on failure.
   */
  X_STRING_API int32_t x_utf8_decode(const char* ptr, const char* end, size_t* out_len);

  /**
   * @brief Returns the expected byte length for a UTF-8 starter byte.
   * @param first_byte Parameter.
   * @return 1..4 for a valid starter byte; negative error code if invalid.
   */
  X_STRING_API int32_t x_utf8_codepoint_length(unsigned char first_byte);

  // -------------------------------------------------------------------------------------
  // Fixed size small string functions
  // -------------------------------------------------------------------------------------

  /**
   * @brief Initializes a fixed-capacity small string from a source C-string.
   * @param s Destination small string.
   * @param str Input C-string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_smallstr_init(XSmallstr* s, const char* str);

  /**
   * @brief Formats text into a small string; truncates if necessary.
   * @param s Destination small string.
   * @param fmt printf-style format string.
   * @param param Variadic arguments list.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_format(XSmallstr* s, const char* fmt, ...);

  /**
   * @brief Copies a null-terminated C-string into a small string.
   * @param s Destination small string.
   * @param cstr Source null-terminated C-string.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_from_cstr(XSmallstr* s, const char* cstr);

  /**
   * @brief Copies a string view into a small string.
   * @param sv String view input.
   * @param out Parameter.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_from_slice(XSlice sv, XSmallstr* out);

  /**
   * @brief Creates a non-owning view over a small string's contents.
   * @param s Destination small string.
   * @return Resulting view after the operation.
   */
  X_STRING_API XSlice  x_smallstr_to_slice(const XSmallstr* s);

  /**
   * @brief Returns the current byte length of the small string.
   * @param s Destination small string.
   * @return Number of elements in the string (excluding the final terminator).
   */
  X_STRING_API size_t    x_smallstr_length(const XSmallstr* s);

  /**
   * @brief Clears the small string to empty.
   * @param s Destination small string.
   * @return No return value.
   */
  X_STRING_API void      x_smallstr_clear(XSmallstr* s);

  /**
   * @brief Appends a null-terminated C-string if space allows.
   * @param s Destination small string.
   * @param cstr Source null-terminated C-string.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_append_cstr(XSmallstr* s, const char* cstr);

  /**
   * @brief Appends a single character if space allows.
   * @param s Destination small string.
   * @param c Character value.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_append_char(XSmallstr* s, char c);

  /**
   * @brief Copies a byte-range substring into a small string.
   * @param s Destination small string.
   * @param start Starting offset (0-based).
   * @param len Number of units to copy or inspect.
   * @param out Parameter.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out);

  /**
   * @brief Trims leading ASCII whitespace in a small string.
   * @param s Destination small string.
   * @return Resulting view after the operation.
   */
  X_STRING_API void      x_smallstr_trim_left(XSmallstr* s);

  /**
   * @brief Trims trailing ASCII whitespace in a small string.
   * @param s Destination small string.
   * @return No return value.
   */
  X_STRING_API void      x_smallstr_trim_right(XSmallstr* s);

  /**
   * @brief Trims leading and trailing ASCII whitespace in a small string.
   * @param s Destination small string.
   * @return Resulting view after the operation.
   */
  X_STRING_API void      x_smallstr_trim(XSmallstr* s);

  /**
   * @brief Compares two small strings case-insensitively.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_smallstr_cmp_ci(const XSmallstr* a, const XSmallstr* b);

  /**
   * @brief Replaces all occurrences of a substring in a small string.
   * @param s Destination small string.
   * @param find Substring to be replaced.
   * @param replace Replacement substring.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_smallstr_replace_all(XSmallstr* s, const char* find, const char* replace);

  /**
   * @brief Counts UTF-8 codepoints in a small string.
   * @param s Destination small string.
   * @return Number of units written or measured (excluding the final terminator, if any).
   */
  X_STRING_API size_t    x_smallstr_utf8_len(const XSmallstr* s);

  /**
   * @brief Compares two small strings.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b);

  /**
   * @brief Compares a small string with a null-terminated C-string.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_smallstr_cmp_cstr(const XSmallstr* a, const char* b);

  /**
   * @brief Returns a pointer to the small string's null-terminated buffer.
   * @param s Destination small string.
   * @return Function-specific result.
   */
  X_STRING_API const char* x_smallstr_cstr(const XSmallstr* s);

  /**
   * @brief Copies a UTF-8 codepoint-range substring into a small string.
   * @param s Destination small string.
   * @param start_cp Starting UTF-8 codepoint offset (0-based).
   * @param len_cp Number of UTF-8 codepoints to copy or inspect.
   * @param out Parameter.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out);

  /**
   * @brief Trims leading Unicode whitespace (by UTF-8 codepoints).
   * @param s Destination small string.
   * @return Resulting view after the operation.
   */
  X_STRING_API void      x_smallstr_utf8_trim_left(XSmallstr* s);

  /**
   * @brief Trims trailing Unicode whitespace (by UTF-8 codepoints).
   * @param s Destination small string.
   * @return No return value.
   */
  X_STRING_API void      x_smallstr_utf8_trim_right(XSmallstr* s);

  /**
   * @brief Trims leading and trailing Unicode whitespace (by UTF-8 codepoints).
   * @param s Destination small string.
   * @return Resulting view after the operation.
   */
  X_STRING_API void      x_smallstr_utf8_trim(XSmallstr* s);

  // -------------------------------------------------------------------------------------
  // XWSmallstr - Wide-char small strting
  // -------------------------------------------------------------------------------------

  /**
   * @brief Initializes a wide-character small string from a wide C-string.
   * @param s Destination small string.
   * @param src Source buffer or string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wsmallstr_init(XWSmallstr* s, const wchar_t* src);

  /**
   * @brief Compares two wide small strings.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b);

  /**
   * @brief Compares a wide small string with a wide C-string.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wsmallstr_cmp_cstr(const XWSmallstr* a, const wchar_t* b);

  /**
   * @brief Compares two wide small strings case-insensitively.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_wsmallstr_cmp_ci(const XWSmallstr* a, const XWSmallstr* b);

  /**
   * @brief Returns the current length (in wide characters).
   * @param s Destination small string.
   * @return Number of elements in the string (excluding the final terminator).
   */
  X_STRING_API size_t    x_wsmallstr_length(const XWSmallstr* s);

  /**
   * @brief Clears the wide small string to empty.
   * @param s Destination small string.
   * @return No return value.
   */
  X_STRING_API void      x_wsmallstr_clear(XWSmallstr* s);

  /**
   * @brief Appends a wide C-string if space allows.
   * @param s Destination small string.
   * @param tail Parameter.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail);

  /**
   * @brief Copies a wide-character substring into a destination small string.
   * @param s Destination small string.
   * @param start Starting offset (0-based).
   * @param len Number of units to copy or inspect.
   * @param out Parameter.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out);

  /**
   * @brief Trims leading and trailing whitespace in a wide small string.
   * @param s Destination small string.
   * @return No return value.
   */
  X_STRING_API void      x_wsmallstr_trim(XWSmallstr* s);

  /**
   * @brief Finds a wide character within a wide small string.
   * @param s Destination small string.
   * @param c Character value.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_wsmallstr_find(const XWSmallstr* s, wchar_t c);

  // -------------------------------------------------------------------------------------
  // XSlice - Non-owning string views and operations.
  // -------------------------------------------------------------------------------------

#define     x_slice_empty() ((XSlice){ .ptr = 0, .length = 0 })
#define     x_slice_is_empty(sv) ((sv).length == 0)
#define     x_slice_init(cstr, len) ((XSlice){ .ptr = (cstr), .length = (len) })
#define     x_slice(cstr)           (x_slice_init( (cstr), strlen(cstr) ))

  /**
   * @brief Compares two string views for equality.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_eq(XSlice a, XSlice b);

  /**
   * @brief Compares a string view with a C-string for equality.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_eq_cstr(XSlice a, const char* b);

  /**
   * @brief Compares two string views case-insensitively.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_eq_ci(XSlice a, XSlice b);

  /**
   * @brief Lexicographically compares two string views.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Negative, zero, or positive according to lexicographic order.
   */
  X_STRING_API int32_t   x_slice_cmp(XSlice a, XSlice b);

  /**
   * @brief Lexicographically compares two string views case-insensitively.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_slice_cmp_ci(XSlice a, XSlice b);

  /**
   * @brief Creates a sub-view by byte range from a string view.
   * @param sv String view input.
   * @param start Starting offset (0-based).
   * @param len Number of units to copy or inspect.
   * @return Resulting view after the operation.
   */
  X_STRING_API XSlice  x_slice_substr(XSlice sv, size_t start, size_t len);

  /**
   * @brief Removes leading ASCII whitespace from a string view.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API XSlice  x_slice_trim_left(XSlice sv);

  /**
   * @brief Removes trailing ASCII whitespace from a string view.
   * @param sv String view input.
   * @return Resulting view after the operation.
   */
  X_STRING_API XSlice  x_slice_trim_right(XSlice sv);

  /**
   * @brief Removes leading and trailing ASCII whitespace from a string view.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API XSlice  x_slice_trim(XSlice sv);

  /**
   * @brief Finds the first occurrence of a character in a string view.
   * @param sv String view input.
   * @param c Character value.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_slice_find(XSlice sv, char c);

  /**
   * @brief Finds the next ASCII whitespace in a string view.
   * @param sv String view input.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_slice_find_white_space(XSlice sv);

  /**
   * @brief Finds the last occurrence of a character in a string view.
   * @param sv String view input.
   * @param c Character value.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_slice_rfind(XSlice sv, char c);

  /**
   * @brief Splits a string view at the first occurrence of a delimiter.
   * @param sv String view input.
   * @param delim Delimiter character or codepoint.
   * @param left Output: left side view.
   * @param right Output: right side view.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_split_at(XSlice sv, char delim, XSlice* left, XSlice* right);

  /**
   * @brief Splits a string view at the first ASCII whitespace.
   * @param sv String view input.
   * @param left Output: left side view.
   * @param right Output: right side view.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_split_at_white_space(XSlice sv, XSlice* left, XSlice* right);

  /**
   * @brief Extracts the next token delimited by ASCII whitespace.
   * @param input Parameter.
   * @param token Output: extracted token.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_next_token_white_space(XSlice* input, XSlice* token);

  /**
   * @brief Extracts the next token delimited by a character.
   * @param input Parameter.
   * @param delim Delimiter character or codepoint.
   * @param token Output: extracted token.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_next_token(XSlice* input, char delim, XSlice* token);

  /**
   * @brief Checks whether a string view starts with the given C-string prefix.
   * @param sv String view input.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_starts_with_cstr(XSlice sv, const char* prefix);

  /**
   * @brief Checks whether a string view ends with the given C-string suffix.
   * @param sv String view input.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_ends_with_cstr(XSlice sv, const char* prefix);

  /**
   * @brief Creates a sub-view by byte range from a string view.
   * @param sv String view input.
   * @param char_start Parameter.
   * @param char_len Parameter.
   * @return Function-specific result.
   */
  X_STRING_API XSlice  x_slice_utf8_substr(XSlice sv, size_t char_start, size_t char_len);

  /**
   * @brief Removes leading Unicode whitespace (by UTF-8 codepoints).
   * @param sv String view input.
   * @return Resulting view after the operation.
   */
  X_STRING_API XSlice  x_slice_utf8_trim_left(XSlice sv);

  /**
   * @brief Removes trailing ASCII whitespace from a string view.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API XSlice  x_slice_utf8_trim_right(XSlice sv);

  /**
   * @brief Removes leading and trailing Unicode whitespace (by UTF-8 codepoints).
   * @param sv String view input.
   * @return Resulting view after the operation.
   */
  X_STRING_API XSlice  x_slice_utf8_trim(XSlice sv);

  /**
   * @brief Finds the first occurrence of a character in a string view.
   * @param sv String view input.
   * @param codepoint Unicode codepoint value.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_slice_utf8_find(XSlice sv, uint32_t codepoint);

  /**
   * @brief Finds the last occurrence of a UTF-8 codepoint in a string view.
   * @param sv String view input.
   * @param codepoint Unicode codepoint value.
   * @return Index on success, or -1 if not found.
   */
  X_STRING_API int32_t   x_slice_utf8_rfind(XSlice sv, uint32_t codepoint);

  /**
   * @brief Splits a string view at the first occurrence of a delimiter.
   * @param sv String view input.
   * @param delim Delimiter character or codepoint.
   * @param left Output: left side view.
   * @param right Output: right side view.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_utf8_split_at(XSlice sv, uint32_t delim, XSlice* left, XSlice* right);

  /**
   * @brief Extracts the next token delimited by a UTF-8 codepoint.
   * @param input Parameter.
   * @param codepoint Unicode codepoint value.
   * @param token Output: extracted token.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_slice_utf8_next_token(XSlice* input, uint32_t codepoint, XSlice* token);

  /**
   * @brief Checks whether a string view starts with the given C-string prefix.
   * @param sv String view input.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_utf8_starts_with_cstr(XSlice sv, const char* prefix);

  /**
   * @brief Checks whether a string view ends with the given C-string suffix.
   * @param sv String view input.
   * @param prefix Prefix to match.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_slice_utf8_ends_with_cstr(XSlice sv, const char* prefix);

  // -------------------------------------------------------------------------------------
  // XWStrview (string views on wchar_t)*
  // Non-owning wide string views and operations.
  // -------------------------------------------------------------------------------------

#define x_wslice_init(cstr, len) ((XWStrview){ .ptr = (cstr), .length = (len) })
#define x_wslice(cstr)           (x_wslice_init( (cstr), wcslen(cstr) ))

  /**
   * @brief Checks whether the wide string view is empty.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_wslice_empty(XWStrview sv);

  /**
   * @brief Compares two wide string views for equality.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return true if the condition holds; false otherwise.
   */
  X_STRING_API bool      x_wslice_eq(XWStrview a, XWStrview b);

  /**
   * @brief Lexicographically compares two wide string views.
   * @param a First input view or string.
   * @param b Second input view or string.
   * @return Function-specific integer result.
   */
  X_STRING_API int32_t   x_wslice_cmp(XWStrview a, XWStrview b);

  /**
   * @brief Creates a sub-view by range from a wide string view.
   * @param sv String view input.
   * @param start Starting offset (0-based).
   * @param len Number of units to copy or inspect.
   * @return Resulting view after the operation.
   */
  X_STRING_API XWStrview x_wslice_substr(XWStrview sv, size_t start, size_t len);
  /**
   * @brief Removes leading whitespace from a wide string view.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API XWStrview x_wslice_trim_left(XWStrview sv);

  /**
   * @brief Removes trailing whitespace from a wide string view.
   * @param sv String view input.
   * @return Resulting view after the operation.
   */
  X_STRING_API XWStrview x_wslice_trim_right(XWStrview sv);

  /**
   * @brief Removes leading and trailing whitespace from a wide string view.
   * @param sv String view input.
   * @return Function-specific result.
   */
  X_STRING_API XWStrview x_wslice_trim(XWStrview sv);

  /**
   * @brief Splits a wide string view at the first occurrence of a delimiter.
   * @param sv String view input.
   * @param delim Delimiter character or codepoint.
   * @param left Output: left side view.
   * @param right Output: right side view.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_wslice_split_at(XWStrview sv, uint32_t delim, XWStrview* left, XWStrview* right);

  /**
   * @brief Extracts the next token from a string view using the given delimiter.
   * @param input Parameter.
   * @param delim Parameter.
   * @param token Output: extracted token.
   * @return Function-specific result.
   */
  X_STRING_API bool      x_wslice_next_token(XWStrview* input, wchar_t delim, XWStrview* token);

  /**
   * @brief Creates a non-owning string view from a null-terminated C-string.
   * @param s Source null-terminated C-string (may be NULL).
   * @return XSlice pointing to the provided string, or an empty view if s is NULL.
   */
  X_STRING_API XSlice x_slice_from_cstr(const char* s);

  /**
   * @brief Creates a non-owning string view from a fixed-capacity small string.
   * @param s Source small string (may be NULL).
   * @return XSlice referencing the contents of the small string, or an empty view if s is NULL.
   */
  X_STRING_API XSlice x_slice_from_smallstr(const XSmallstr* s);

  /**
   * @brief Appends the contents of a string view to a small string.
   * @param s Destination small string.
   * @param sv Source string view to append.
   * @return New length of the small string in bytes after the operation (null-terminated).
   */
  X_STRING_API size_t x_smallstr_append_slice(XSmallstr* s, XSlice sv);

  /**
   * @brief Appends a fixed-length portion of a C-string to a small string.
   * @param s Destination small string.
   * @param cstr Source C-string (may be NULL).
   * @param n Number of bytes to append from cstr.
   * @return New length of the small string in bytes after the operation.
   */
  X_STRING_API size_t x_smallstr_append_n(XSmallstr* s, const char* cstr, size_t n);

  /**
   * @brief Appends formatted text to a small string using a va_list.
   * @param s Destination small string.
   * @param fmt printf-style format string.
   * @param args va_list argument list.
   * @return New length of the small string in bytes after the operation.
   */
  X_STRING_API size_t x_smallstr_vappendf(XSmallstr* s, const char* fmt, va_list args);

  /**
   * @brief Appends formatted text to a small string.
   * @param s Destination small string.
   * @param fmt printf-style format string.
   * @param ... Additional arguments for formatting.
   * @return New length of the small string in bytes after the operation.
   */
  X_STRING_API size_t x_smallstr_appendf(XSmallstr* s, const char* fmt, ...);

  /**
   * @brief Checks whether a string view contains the specified character.
   * @param sv Input string view.
   * @param c Character to search for.
   * @return true if the character is found; false otherwise.
   */
  X_STRING_API bool x_slice_contains_char(XSlice sv, char c);

  /**
   * @brief Checks whether a string view contains the specified UTF-8 codepoint.
   * @param sv Input string view.
   * @param codepoint Unicode codepoint to search for.
   * @return true if the codepoint is found; false otherwise.
   */
  X_STRING_API bool x_slice_contains_utf8(XSlice sv, uint32_t codepoint);

  /**
   * @brief Checks whether a small string contains the specified character.
   * @param s Input small string.
   * @param c Character to search for.
   * @return true if the character is found; false otherwise.
   */
  X_STRING_API bool x_smallstr_contains_char(const XSmallstr* s, char c);

  /**
   * @brief Concatenates multiple string views into a small string, inserting a separator between them.
   * @param dst Destination small string.
   * @param parts Array of string views to join.
   * @param count Number of elements in the parts array.
   * @param sep Separator view inserted between joined elements.
   * @return New length of the small string after concatenation.
   */
  X_STRING_API size_t x_smallstr_join(XSmallstr* dst, const XSlice* parts, size_t count, XSlice sep);

  /**
   * @brief Returns the maximum number of bytes (excluding terminator) a small string can hold.
   * @return The compile-time capacity constant X_SMALLSTR_MAX_LENGTH.
   */
  X_STRING_API size_t x_smallstr_capacity(void);

  /**
   * @brief Tests whether a small string is empty.
   * @param s Input small string.
   * @return true if the small string has zero length or is NULL; false otherwise.
   */
  X_STRING_API bool x_smallstr_is_empty(const XSmallstr* s);

  /**
   * @brief Attempts to append a C-string to a small string up to its capacity.
   * @param s Destination small string.
   * @param cstr Source null-terminated C-string.
   * @param out_appended Optional pointer to receive bytes actually appended.
   * @return true if any bytes were appended; false if none or on error.
   */
  X_STRING_API bool x_smallstr_try_append_cstr(XSmallstr* s, const char* cstr, size_t* out_appended);

  /**
   * @brief Creates a compile-time string view from a string literal.
   * @param s String literal.
   * @return XSlice representing the literal contents, excluding the null terminator.
   */
#define X_STRVIEW(s) ((XSlice){ (s), sizeof(s) - 1u })


#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_STRING

#include <string.h>  /* strlen, memcpy, memmove (used by helpers/impl) */
#include <wchar.h>   /* wchar_t APIs (XW* types) */
#include <ctype.h>   /* tolower */
#include <wctype.h>
#include <locale.h>
#include <stdio.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#define strnicmp _strnicmp
#endif

static bool s_is_unicode_whitespace(uint32_t cp)
{
  return (cp == 0x09 || cp == 0x0A || cp == 0x0B || cp == 0x0C || cp == 0x0D || cp == 0x20 ||
      cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
      (cp >= 0x2000 && cp <= 0x200A) ||
      cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000);
}

static int32_t utf8_decode(const char** ptr, const char* end)
{
  const char* s = *ptr;

  if (s == NULL || end == NULL)
  {
    return X_UTF8_ERR_INVALID;
  }
  if (s >= end)
  {
    return X_UTF8_ERR_EOF;
  }

  const unsigned char b0 = (unsigned char)s[0];
  size_t need = 0;
  uint32_t cp = 0;

  if (b0 < 0x80u)
  {
    cp = b0;
    need = 1;
  }
  else if ((b0 & 0xE0u) == 0xC0u)
  {
    cp = (b0 & 0x1Fu);
    need = 2;
  }
  else if ((b0 & 0xF0u) == 0xE0u)
  {
    cp = (b0 & 0x0Fu);
    need = 3;
  }
  else if ((b0 & 0xF8u) == 0xF0u)
  {
    cp = (b0 & 0x07u);
    need = 4;
  }
  else
  {
    /* Invalid starter: skip 1 */
    *ptr = s + 1;
    return X_UTF8_ERR_INVALID;
  }

  /* Not enough bytes available for the full sequence */
  {
    size_t avail = (size_t)(end - s);
    if (avail < need)
    {
      /* Truncated near end: skip what's left to make forward progress */
      *ptr = s + avail;
      return X_UTF8_ERR_INVALID;
    }
  }

  /* Validate continuation bytes and assemble */
  for (size_t i = 1; i < need; ++i)
  {
    const unsigned char bi = (unsigned char)s[i];
    if ((bi & 0xC0u) != 0x80u)
    {
      /* Bad continuation: skip only the starter so the next byte can be re-interpreted */
      *ptr = s + 1;
      return X_UTF8_ERR_INVALID;
    }
    cp = (cp << 6) | (uint32_t)(bi & 0x3Fu);
  }

  /* Mask to final code point (header bits already folded in by shifts above) */
  if (need == 2) cp &= 0x07FFu;
  if (need == 3) cp &= 0xFFFFu;
  if (need == 4) cp &= 0x1FFFFFu;

  /* Overlong checks (must be at least the minimum encodable value for its length) */
  if ((need == 2 && cp < 0x80u) || (need == 3 && cp < 0x800u) || (need == 4 && cp < 0x10000u))
  {
    *ptr = s + need; /* drop the entire malformed sequence */
    return X_UTF8_ERR_OVERLONG;
  }

  /* Disallow UTF-16 surrogates and out-of-range values */
  if ((cp >= 0xD800u && cp <= 0xDFFFu) || cp > 0x10FFFFu)
  {
    *ptr = s + need;
    return X_UTF8_ERR_RANGE;
  }

  /* Success */
  *ptr = s + need;
  return (int32_t)cp;
}

static size_t utf8_advance(const char* s, size_t len, size_t chars)
{
  size_t i = 0, count = 0;
  while (i < len && count < chars)
  {
    char c = (char)s[i];
    if ((c & 0x80) == 0x00) i += 1;
    else if ((c & 0xE0) == 0xC0) i += 2;
    else if ((c & 0xF0) == 0xE0) i += 3;
    else if ((c & 0xF8) == 0xF0) i += 4;
    else break;
    count++;
  }
  return i;
}

X_STRING_API char* x_cstr_str(const char *haystack, const char *needle)
{
  return strstr(haystack, needle);
}

X_STRING_API bool x_cstr_ends_with(const char* str, const char* suffix)
{
  const char* found = strstr(str, suffix);
  if (found == NULL || found + strlen(suffix) != str + strlen(str))


  {
    return false;
  }
  return true;
}

X_STRING_API bool x_cstr_starts_with(const char* str, const char* prefix)
{
  const char* found = strstr(str, prefix);
  bool match = (found == str);
  if (match && prefix[0] == 0 && str[0] != 0)
    match = false;

  return match;
}

X_STRING_API bool x_cstr_starts_with_ci(const char* str, const char* prefix)
{
  const char* found = x_cstr_str(str, prefix);
  bool match = (found == str);
  if (match && prefix[0] == 0 && str[0] != 0)
    match = false;

  return match;
}

X_STRING_API bool x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix)
{
  size_t plen = wcslen(prefix);
  return wcsncmp(str, prefix, plen) == 0;
}

X_STRING_API bool x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix)
{
  size_t slen = wcslen(str);
  size_t suf_len = wcslen(suffix);
  if (slen < suf_len) return false;
  return wcscmp(str + slen - suf_len, suffix) == 0;
}

X_STRING_API bool x_wcstr_starts_with_ci(const wchar_t* str, const wchar_t* prefix)
{
  while (*prefix && *str)
  {
    if (towlower(*str++) != towlower(*prefix++))
      return false;
  }
  return *prefix == 0;
}

X_STRING_API bool x_wcstr_ends_with_ci(const wchar_t* str, const wchar_t* suffix)
{
  size_t slen = wcslen(str);
  size_t suflen = wcslen(suffix);
  if (slen < suflen) return false;

  const wchar_t* str_tail = str + slen - suflen;
  while (*suffix)
  {
    if (towlower(*str_tail++) != towlower(*suffix++))
      return false;
  }
  return true;
}

X_STRING_API int32_t  x_wcstr_cicmp(const wchar_t* a, const wchar_t* b)
{
  while (*a && *b)
  {
    wint_t ca = towlower(*a++);
    wint_t cb = towlower(*b++);
    if (ca != cb) return ca - cb;
  }
  return towlower(*a) - towlower(*b);
}

// Ensure locale is set before using this
X_STRING_API size_t x_cstr_to_wcstr(const char* src, wchar_t* dest, size_t max)
{
  if (!src || !dest || max == 0) return 0;
  mbstate_t state =
  {0};
  size_t len = mbsrtowcs(dest, &src, max - 1, &state);
  if (len == (size_t)-1)
  {
    dest[0] = L'\0';
    return 0;
  }
  dest[len] = L'\0';
  return len;
}

X_STRING_API size_t x_wcstr_to_utf8(const wchar_t* wide, char* utf8, size_t max)
{
  mbstate_t ps = {0};
  return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
}

X_STRING_API size_t x_wcstr_to_cstr(const wchar_t* src, char* dest, size_t max)
{
  if (!src || !dest || max == 0) return 0;
  mbstate_t state =
  {0};
  size_t len = wcsrtombs(dest, &src, max - 1, &state);
  if (len == (size_t)-1)
  {
    dest[0] = '\0';
    return 0;
  }
  dest[len] = '\0';
  return len;
}

X_STRING_API bool x_cstr_ends_with_ci(const char* str, const char* suffix)
{
  const char* found = x_cstr_str(str, suffix);
  if (found == NULL || found + strlen(suffix) != str + strlen(str))
  {
    return false;
  }
  return true;
}

X_STRING_API void x_set_locale(const char* locale)
{
  setlocale(LC_ALL, locale ? locale : "");
}

X_STRING_API size_t x_utf8_strlen(const char* utf8)
{
  size_t count = 0;
  while (*utf8)
  {
    char c = (char)*utf8;
    if ((c & 0x80) == 0x00) utf8 += 1;
    else if ((c & 0xE0) == 0xC0) utf8 += 2;
    else if ((c & 0xF0) == 0xE0) utf8 += 3;
    else if ((c & 0xF8) == 0xF0) utf8 += 4;
    else break;
    count++;
  }
  return count;
}

X_STRING_API int32_t  x_utf8_strcmp(const char* a, const char* b)
{
  if (setlocale(LC_ALL, NULL) != NULL) return strcoll(a, b);
  return strcmp(a, b); // no locale set, fallback to pure ascii comparisons
}

X_STRING_API char* x_utf8_tolower(char* dest, const char* src)
{
  wchar_t wbuf[512];
  x_utf8_to_wcstr(src, wbuf, 512);
  for (size_t i = 0; wbuf[i]; ++i)
    wbuf[i] = towlower(wbuf[i]);
  x_wcstr_to_utf8(wbuf, dest, 512);
  return dest;
}

X_STRING_API char* x_utf8_toupper(char* dest, const char* src)
{
  wchar_t wbuf[512];
  x_utf8_to_wcstr(src, wbuf, 512);
  for (size_t i = 0; wbuf[i]; ++i)
    wbuf[i] = towupper(wbuf[i]);
  x_wcstr_to_utf8(wbuf, dest, 512);
  return dest;
}

X_STRING_API int32_t x_utf8_codepoint_length(unsigned char first_byte)
{
  if (first_byte < 0x80)                return 1;
  else if ((first_byte & 0xE0) == 0xC0) return 2;
  else if ((first_byte & 0xF0) == 0xE0) return 3;
  else if ((first_byte & 0xF8) == 0xF0) return 4;
  else return X_UTF8_ERR_INVALID;
}

X_STRING_API size_t x_utf8_to_wcstr(const char* utf8, wchar_t* wide, size_t max)
{
  mbstate_t ps = {0};
  return mbsrtowcs(wide, &utf8, max, &ps);
}

X_STRING_API int32_t x_utf8_decode(const char* ptr, const char* end, size_t* out_len)
{
  const char* p = ptr;
  const int32_t r = utf8_decode(&p, end);

  if (out_len != NULL)
  {
    if (r >= 0)
    {
      // report codepoints consumed (always 1) 
      *out_len = 1;
    }
    else
    {
      // Error: report how many bytes we skipped so callers can advance
      *out_len = (size_t)(p - ptr);
    }
  }
  return r;
}

X_STRING_API bool x_utf8_is_single_char(const char* s)
{
  const char* end = s + strlen(s);
  const char* p = s;
  uint32_t cp = utf8_decode(&p, end);
  return *p == '\0' && cp != 0;
}

X_STRING_API size_t x_wide_to_utf8(const wchar_t* wide, char* utf8, size_t max)
{
  mbstate_t ps =
  {0};
  return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
}

X_STRING_API uint32_t x_cstr_hash(const char* str)
{
  uint32_t hash = 2166136261u; // FNV offset basis
  while (*str)
  {
    hash ^= (unsigned int) *str++;
    hash *= 16777619u; // FNV prime
  }
  return hash;
}

X_STRING_API size_t smallstr(XSmallstr* smallString, const char* str)
{
#if defined(_DEBUG) || defined(DEBUG)
#if defined(X_x_smallstr_ENABLE_DBG_MESSAGES)
  size_t length = strlen(str);
  if (length >= X_SMALLSTR_MAX_LENGTH)
  {
    log_print_debug("The string length (%d) exceeds the maximum size of a XSmallstr (%d)", length, X_SMALLSTR_MAX_LENGTH);
  }
#endif
#endif
  strncpy((char *) &smallString->buf, str, X_SMALLSTR_MAX_LENGTH - 1);
  smallString->buf[X_SMALLSTR_MAX_LENGTH - 1] = 0;
  smallString->length = strlen(smallString->buf);
  return smallString->length;
}

X_STRING_API size_t x_smallstr_format(XSmallstr* smallString, const char* fmt, ...)
{
  va_list argList;
  va_start(argList, fmt);
  smallString->length = vsnprintf(smallString->buf, X_SMALLSTR_MAX_LENGTH, fmt, argList);
  va_end(argList);

  if (smallString->length >= X_SMALLSTR_MAX_LENGTH)
    smallString->length = X_SMALLSTR_MAX_LENGTH; /* truncated to capacity */
  smallString->buf[smallString->length] = '\0';
  return smallString->length;
}

X_STRING_API size_t x_smallstr_from_cstr(XSmallstr* s, const char* cstr)
{
  size_t len = strlen(cstr);
  if (len > X_SMALLSTR_MAX_LENGTH)
    return 0;
  memcpy(s->buf, cstr, len);
  s->buf[len] = '\0';
  s->length = len;
  return len;
}

X_STRING_API const char* x_smallstr_cstr(const XSmallstr* s)
{
  ((char*)s->buf)[s->length] = '\0';
  return (const char*)s->buf;
}

X_STRING_API size_t x_smallstr_length(const XSmallstr* smallString)
{
  return smallString->length;
}

X_STRING_API void x_smallstr_clear(XSmallstr* smallString)
{
  memset(smallString->buf, 0, X_SMALLSTR_MAX_LENGTH * sizeof(char));
  smallString->length = 0;
}

X_STRING_API size_t x_smallstr_append_cstr(XSmallstr* s, const char* cstr)
{
  size_t len = strlen(cstr);
  if (s->length + len > X_SMALLSTR_MAX_LENGTH) return 0;
  memcpy(&s->buf[s->length], cstr, len);
  s->length += len;
  s->buf[s->length] = '\0';
  return s->length;
}

X_STRING_API size_t x_smallstr_append_char(XSmallstr* s, char c)
{
  if (s->length + 1 > X_SMALLSTR_MAX_LENGTH) return 0;
  s->buf[s->length++] = (char)c;
  s->buf[s->length] = '\0';
  return s->length;
}

X_STRING_API size_t x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out)
{
  if (start > s->length || start + len > s->length) return 0;
  out->length = len;
  memcpy(out->buf, &s->buf[start], len);
  out->buf[len] = '\0';
  return len;
}

X_STRING_API void x_smallstr_trim_left(XSmallstr* s)
{
  size_t i = 0;
  while (i < s->length && isspace(s->buf[i])) i++;
  if (i > 0)
  {
    memmove(s->buf, &s->buf[i], s->length - i);
    s->length -= i;
    s->buf[s->length] = '\0';
  }
}

X_STRING_API void x_smallstr_trim_right(XSmallstr* s)
{
  while (s->length > 0 && isspace(s->buf[s->length - 1]))
  {
    s->length--;
  }
  s->buf[s->length] = '\0';
}

X_STRING_API void x_smallstr_trim(XSmallstr* s)
{
  x_smallstr_trim_right(s);
  x_smallstr_trim_left(s);
}

X_STRING_API int32_t x_smallstr_cmp_ci(const XSmallstr* a, const XSmallstr* b)
{
  size_t len = (a->length > b->length) ? a->length : b->length;
  return strnicmp(a->buf, b->buf, len);
}

X_STRING_API int32_t x_smallstr_replace_all(XSmallstr* s, const char* find, const char* replace)
{
  XSmallstr result;
  x_smallstr_clear(&result);

  size_t find_len = strlen(find);
  size_t replace_len = strlen(replace);
  size_t i = 0;

  while (i < s->length)
  {
    if (i + find_len <= s->length && memcmp(&s->buf[i], find, find_len) == 0)
    {
      if (result.length + replace_len > X_SMALLSTR_MAX_LENGTH)
        return -1;
      memcpy(&result.buf[result.length], replace, replace_len);
      result.length += replace_len;
      i += find_len;
    }
    else
    {
      if (result.length + 1 > X_SMALLSTR_MAX_LENGTH)
        return -1;
      result.buf[result.length++] = s->buf[i++];
    }
  }

  result.buf[result.length] = '\0';
  *s = result;
  return 0;
}

X_STRING_API size_t x_smallstr_utf8_len(const XSmallstr* s)
{
  size_t count = 0;
  for (size_t i = 0; i < s->length;)
  {
    char c = s->buf[i];
    if ((c & 0x80) == 0x00) i += 1;
    else if ((c & 0xE0) == 0xC0) i += 2;
    else if ((c & 0xF0) == 0xE0) i += 3;
    else if ((c & 0xF8) == 0xF0) i += 4;
    else return count; // invalid byte
    count++;
  }
  return count;
}

X_STRING_API size_t x_smallstr_from_slice(XSlice sv, XSmallstr* out)
{
  x_smallstr_clear(out);
  if (sv.length > X_SMALLSTR_MAX_LENGTH) return 0;
  memcpy(out->buf, sv.ptr, sv.length);
  out->buf[sv.length] = '\0';
  out->length = sv.length;
  return out->length;
}

X_STRING_API int32_t x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b)
{
  return strncmp(a->buf, b->buf, b->length);
}

X_STRING_API int32_t x_smallstr_cmp_cstr(const XSmallstr* a, const char* b)
{
  return strncmp(a->buf, b, a->length);
}

X_STRING_API int32_t x_smallstr_find(const XSmallstr* s, char c)
{
  for (size_t i = 0; i < s->length; ++i)
  {
    if (s->buf[i] == c) return (int)i;
  }
  return -1;
}

X_STRING_API int32_t x_smallstr_rfind(const XSmallstr* s, char c)
{
  for (size_t i = s->length; i > 0; --i)
  {
    if (s->buf[i - 1] == c) return (int)(i - 1);
  }
  return -1;
}

X_STRING_API int32_t x_smallstr_split_at(const XSmallstr* s, char delim, XSmallstr* left, XSmallstr* right)
{
  int32_t pos = x_smallstr_find(s, delim);
  if (pos < 0) return 0;
  x_smallstr_substring(s, 0, pos, left);
  x_smallstr_substring(s, pos + 1, s->length - pos - 1, right);
  return 1;
}

X_STRING_API XSlice x_smallstr_to_slice(const XSmallstr* s)
{
  return x_slice_init(s->buf, s->length);
}

X_STRING_API int32_t x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out)
{
  size_t start = utf8_advance(s->buf, s->length, start_cp);
  size_t len = utf8_advance(s->buf + start, s->length - start, len_cp);
  if (start + len > s->length) return -1;

  out->length = len;
  memcpy(out->buf, s->buf + start, len);
  out->buf[len] = '\0';
  return 0;
}

X_STRING_API void x_smallstr_utf8_trim_left(XSmallstr* s)
{
  const char* start = s->buf;
  const char* end = s->buf + s->length;

  while (start < end)
  {
    const char* prev = start;
    uint32_t cp = utf8_decode(&start, end);
    if (!s_is_unicode_whitespace(cp))
    {
      memmove(s->buf, prev, end - prev);
      s->length = end - prev;
      s->buf[s->length] = '\0';
      return;
    }
  }
  x_smallstr_clear(s);
}

X_STRING_API void x_smallstr_utf8_trim_right(XSmallstr* s)
{
  const char* start = s->buf;
  const char* end = s->buf + s->length;
  const char* p = end;

  while (p > start)
  {
    const char* prev = p;
    do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
    const char* decode_from = p;
    uint32_t cp = utf8_decode(&decode_from, end);
    if (!s_is_unicode_whitespace(cp))
    {
      size_t new_len = prev - s->buf;
      s->length = new_len;
      s->buf[new_len] = '\0';
      return;
    }
  }
  x_smallstr_clear(s);
}

X_STRING_API void x_smallstr_utf8_trim(XSmallstr* s)
{
  x_smallstr_utf8_trim_right(s);
  x_smallstr_utf8_trim_left(s);
}

X_STRING_API int32_t x_wsmallstr_from_wcstr(XWSmallstr* s, const wchar_t* src)
{
  size_t len = wcslen(src);
  if (len > X_SMALLSTR_MAX_LENGTH) return -1;
  wcsncpy(s->buf, src, X_SMALLSTR_MAX_LENGTH);
  s->buf[len] = L'\0';
  s->length = len;
  return 0;
}

X_STRING_API int32_t x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b)
{
  return wcsncmp(a->buf, b->buf, a->length);
}

X_STRING_API int32_t x_wsmallstr_cmp_cstr(const XWSmallstr* a, const wchar_t* b)
{
  return wcsncmp(a->buf, b, wcslen(b));
}

X_STRING_API int32_t x_wsmallstr_cmp_ci(const XWSmallstr* a, const XWSmallstr* b)
{
  if (a->length != b->length) return 1;
  for (size_t i = 0; i < a->length; ++i)
  {
    if (towlower(a->buf[i]) != towlower(b->buf[i])) return 1;
  }
  return 0;
}

X_STRING_API size_t x_wsmallstr_length(const XWSmallstr* s)
{
  return s->length;
}

X_STRING_API void x_wsmallstr_clear(XWSmallstr* s)
{
  s->length = 0;
  s->buf[0] = L'\0';
}

X_STRING_API int32_t x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail)
{
  size_t tail_len = wcslen(tail);
  if (s->length + tail_len > X_SMALLSTR_MAX_LENGTH)
    return -1;

  wcsncpy(&s->buf[s->length], tail, tail_len);
  s->length += tail_len;
  s->buf[s->length] = L'\0';
  return 0;
}

X_STRING_API int32_t x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out)
{
  if (start > s->length || (start + len) > s->length)
    return -1;

  wcsncpy(out->buf, &s->buf[start], len);
  out->buf[len] = L'\0';
  out->length = len;
  return 0;
}

X_STRING_API void x_wsmallstr_trim(XWSmallstr* s)
{
  size_t start = 0;
  size_t end = s->length;

  // Find first non-space
  while (start < end && iswspace(s->buf[start]))
    start++;

  // Find last non-space
  while (end > start && iswspace(s->buf[end - 1]))
    end--;

  size_t new_len = end - start;

  if (start > 0 && new_len > 0)
    wmemmove(s->buf, s->buf + start, new_len);

  s->length = new_len;
  s->buf[new_len] = L'\0';
}

X_STRING_API int32_t x_wsmallstr_find(const XWSmallstr* s, wchar_t c)
{
  for (size_t i = 0; i < s->length; ++i)
  {
    if (s->buf[i] == c)
      return (int32_t)i;
  }
  return -1;
}

X_STRING_API int32_t x_wsmallstr_next_token(XWSmallstr* input, wchar_t delim, XWSmallstr* token)
{
  XWSmallstr temp;
  int32_t found = 0;
  int32_t pos = x_wsmallstr_find(input, delim);
  if (pos >= 0)
  {
    x_wsmallstr_substring(input, 0, pos, token);
    x_wsmallstr_substring(input, pos + 1, input->length - pos - 1, &temp);
    *input = temp;
    found = 1;
  }
  else if (input->length > 0)
  {
    *token = *input;
    x_wsmallstr_clear(input);
    found = 1;
  }
  return found;
}

X_STRING_API bool x_slice_eq(XSlice a, XSlice b)
{
  return a.length == b.length && (memcmp(a.ptr, b.ptr, a.length) == 0);
}

X_STRING_API bool x_slice_eq_cstr(XSlice a, const char* b)
{
  return x_slice_eq(a, x_slice(b));
}

X_STRING_API int32_t x_slice_cmp(XSlice a, XSlice b)
{
  size_t min_len = a.length < b.length ? a.length : b.length;
  int32_t r = memcmp(a.ptr, b.ptr, min_len);
  if (r != 0) return r;
  return (int)(a.length - b.length);
}

X_STRING_API bool x_slice_eq_ci(XSlice a, XSlice b)
{
  if (a.length != b.length) return 0;
  for (size_t i = 0; i < a.length; i++)
  {
    if (tolower((unsigned char)a.ptr[i]) != tolower((unsigned char)b.ptr[i]))
      return false;
  }
  return true;
}

X_STRING_API int32_t x_slice_cmp_ci(XSlice a, XSlice b)
{
  size_t min_len = a.length < b.length ? a.length : b.length;
  for (size_t i = 0; i < min_len; i++)
  {
    int32_t ca = tolower((unsigned char)a.ptr[i]);
    int32_t cb = tolower((unsigned char)b.ptr[i]);
    if (ca != cb) return ca - cb;
  }
  return (int)(a.length - b.length);
}

X_STRING_API XSlice x_slice_substr(XSlice sv, size_t start, size_t len)
{
  if (start > sv.length) start = sv.length;
  if (start + len > sv.length) len = sv.length - start;
  return x_slice_init(sv.ptr + start, len);
}

X_STRING_API XSlice x_slice_trim_left(XSlice sv)
{
  size_t i = 0;
  while (i < sv.length && (unsigned char)sv.ptr[i] <= ' ') i++;
  return x_slice_substr(sv, i, sv.length - i);
}

X_STRING_API XSlice x_slice_trim_right(XSlice sv)
{
  size_t i = sv.length;
  while (i > 0 && (unsigned char)sv.ptr[i - 1] <= ' ') i--;
  return x_slice_substr(sv, 0, i);
}

X_STRING_API XSlice x_slice_trim(XSlice sv)
{
  return x_slice_trim_right(x_slice_trim_left(sv));
}

X_STRING_API int32_t x_slice_find(XSlice sv, char c)
{
  for (size_t i = 0; i < sv.length; i++)
  {
    if (sv.ptr[i] == c) return (int)i;
  }
  return -1;
}

X_STRING_API int32_t x_slice_find_white_space(XSlice sv)
{
  for (size_t i = 0; i < sv.length; i++)
  {
    if ( sv.ptr[i] == ' ' ||
        sv.ptr[i] == '\t' ||
        sv.ptr[i] == '\r'
       ) return (int)i;
  }
  return -1;
}

X_STRING_API int32_t x_slice_rfind(XSlice sv, char c)
{
  for (size_t i = sv.length; i > 0; i--)
  {
    if (sv.ptr[i - 1] == c) return (int)(i - 1);
  }
  return -1;
}

X_STRING_API bool x_slice_split_at(XSlice sv, char delim, XSlice* left, XSlice* right)
{
  int32_t pos = x_slice_find(sv, delim);
  if (pos < 0) return false;
  if (left) *left = x_slice_substr(sv, 0, pos);
  if (right) *right = x_slice_substr(sv, pos + 1, sv.length - pos - 1);
  return true;
}

static bool inline s_char_is_white_space(char c)
{
  return ( c == ' ' || c == '\t' || c == '\r');
}

X_STRING_API bool x_slice_split_at_white_space(XSlice sv, XSlice* left, XSlice* right)
{
  int32_t pos = x_slice_find_white_space(sv);

  if (pos < 0) return false;
  if (left)
    *left = x_slice_trim(x_slice_substr(sv, 0, pos));
  if (right)
    *right = x_slice_trim(x_slice_substr(sv, pos + 1, sv.length - pos - 1));
  return true;
}

X_STRING_API bool x_slice_next_token_white_space(XSlice* input, XSlice* token)
{
  XSlice rest;
  if (x_slice_split_at_white_space(*input, token, &rest))
  {
    *input = rest;
    return true;
  }
  else if (input->length > 0)
  {
    *token = *input;
    *input = (XSlice){0};
    return true;
  }

  token->ptr = NULL;
  token->length = 0;
  return false;
}

X_STRING_API bool x_slice_next_token(XSlice* input, char delim, XSlice* token)
{
  XSlice rest;
  if (x_slice_split_at(*input, delim, token, &rest))
  {
    *input = rest;
    return true;
  }
  else if (input->length > 0)
  {
    *token = *input;
    *input = (XSlice){0};
    return true;
  }

  token->ptr = NULL;
  token->length = 0;
  return false;
}

X_STRING_API bool x_slice_starts_with_cstr(XSlice sv, const char* prefix)
{
  if (prefix == NULL || sv.length == 0 || sv.ptr == NULL)
    return false;
  size_t prefix_len = strlen(prefix);
  if (prefix_len > sv.length)
    return false;

  return strncmp(sv.ptr, prefix, prefix_len) == 0;
}

X_STRING_API bool x_slice_ends_with_cstr(XSlice sv, const char* suffix)
{
  if (suffix == NULL || sv.length == 0 || sv.ptr == NULL)
    return false;
  size_t suffix_len = strlen(suffix);
  if (suffix_len > sv.length)
    return false;

  return strncmp(sv.ptr + sv.length - suffix_len, suffix, suffix_len) == 0;
}

X_STRING_API bool x_wslice_empty(XWStrview sv)
{
  return sv.length == 0;
}

X_STRING_API bool x_wslice_eq(XWStrview a, XWStrview b)
{
  return a.length == b.length && wcsncmp(a.ptr, b.ptr, a.length) == 0;
}

X_STRING_API int32_t x_wslice_cmp(XWStrview a, XWStrview b)
{
  size_t min = a.length < b.length ? a.length : b.length;
  int32_t result = wcsncmp(a.ptr, b.ptr, min);
  return result != 0 ? result : (int)(a.length - b.length);
}

X_STRING_API XWStrview x_wslice_substr(XWStrview sv, size_t start, size_t len)
{
  if (start > sv.length) start = sv.length;
  if (start + len > sv.length) len = sv.length - start;
  return (XWStrview){ sv.ptr + start, len };
}

X_STRING_API XWStrview x_wslice_trim_left(XWStrview sv)
{
  size_t i = 0;
  while (i < sv.length && iswspace(sv.ptr[i])) i++;
  return x_wslice_substr(sv, i, sv.length - i);
}

X_STRING_API XWStrview x_wslice_trim_right(XWStrview sv)
{
  size_t i = sv.length;
  while (i > 0 && iswspace(sv.ptr[i - 1])) i--;
  return x_wslice_substr(sv, 0, i);
}

X_STRING_API XWStrview x_wslice_trim(XWStrview sv)
{
  return x_wslice_trim_right(x_wslice_trim_left(sv));
}

X_STRING_API bool x_wslice_split_at(XWStrview sv, uint32_t delim, XWStrview* left, XWStrview* right)
{
  for (size_t i = 0; i < sv.length; ++i)
  {
    if ((uint32_t)sv.ptr[i] == delim)
    {
      if (left) {
        left->ptr = sv.ptr;
        left->length = i;
      }
      if (right)
      {
        right->ptr = sv.ptr + i + 1;
        right->length = sv.length - i - 1;
      }
      return true;
    }
  }
  return false;
}

X_STRING_API bool x_wslice_next_token(XWStrview* input, wchar_t delim, XWStrview* token)
{
  XWStrview left, right;
  if (x_wslice_split_at(*input, (uint32_t)delim, &left, &right))
  {
    *token = left;
    *input = right;
    return true;
  }
  else if (input->length > 0)
  {
    *token = *input;
    input->ptr += input->length;
    input->length = 0;
    return true;
  }

  token->ptr = NULL;
  token->length = 0;
  return false;
}

X_STRING_API XSlice x_slice_utf8_substr(XSlice sv, size_t char_start, size_t char_len)
{
  size_t byte_start = utf8_advance(sv.ptr, sv.length, char_start);
  size_t byte_end = utf8_advance(sv.ptr + byte_start, sv.length - byte_start, char_len);
  return x_slice_init(sv.ptr + byte_start, byte_end );
}

X_STRING_API XSlice x_slice_utf8_trim_left(XSlice sv)
{
  const char* start = sv.ptr;
  const char* end = sv.ptr + sv.length;
  while (start < end)
  {
    const char* prev = start;
    uint32_t cp = utf8_decode(&start, end);
    if (!s_is_unicode_whitespace(cp))
    {
      return (XSlice){ prev, (size_t)(end - prev) };
    }
  }
  return x_slice_init(end, 0);
}

X_STRING_API XSlice x_slice_utf8_trim_right(XSlice sv)
{
  const char* start = sv.ptr;
  const char* end = sv.ptr + sv.length;
  const char* p = end;
  while (p > start)
  {
    const char* prev = p;
    // Step back 1 byte to guess start of char
    do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
    const char* decode_from = p;
    uint32_t cp = utf8_decode(&decode_from, end);
    if (!s_is_unicode_whitespace(cp))
    {
      return x_slice_init(sv.ptr, (size_t)(prev - sv.ptr));
    }
  }
  return x_slice_init(sv.ptr + sv.length, 0 );
}

X_STRING_API XSlice x_slice_utf8_trim(XSlice sv)
{
  return x_slice_utf8_trim_right(x_slice_utf8_trim_left(sv));
}

X_STRING_API int32_t x_slice_utf8_find(XSlice sv, uint32_t codepoint)
{
  const char* ptr = sv.ptr;
  const char* end = sv.ptr + sv.length;

  while (ptr < end)
  {
    const char* start = ptr;
    uint32_t cp = utf8_decode(&ptr, end);
    if (cp == codepoint)
      return (int)(start - sv.ptr);
  }
  return -1;
}

X_STRING_API int32_t x_slice_utf8_rfind(XSlice sv, uint32_t codepoint)
{
  const char* ptr = sv.ptr;
  const char* end = sv.ptr + sv.length;
  const char* last_match = NULL;

  while (ptr < end)
  {
    const char* current = ptr;
    uint32_t cp = utf8_decode(&ptr, end);
    if (cp == codepoint)
      last_match = current;
  }

  return last_match ? (int)(last_match - sv.ptr) : -1;
}

X_STRING_API bool x_slice_utf8_split_at(XSlice sv, uint32_t delim, XSlice* left, XSlice* right)
{
  const char* ptr = sv.ptr;
  const char* end = sv.ptr + sv.length;

  while (ptr < end) {
    const char* codepoint_start = ptr;
    int32_t cp = utf8_decode(&ptr, end);

    if (cp == (int32_t)delim) {
      size_t left_len = codepoint_start - sv.ptr;
      size_t right_len = end - ptr;
      *left  = x_slice_init(sv.ptr, left_len);
      *right = x_slice_init(ptr, right_len);
      return true;
    }
  }

  return false;
}

X_STRING_API bool x_slice_utf8_next_token(XSlice* input, uint32_t delim, XSlice* token)
{
  XSlice rest;
  if (x_slice_utf8_split_at(*input, delim, token, &rest)) {
    *input = rest;
    return true;
  }
  if (input->length > 0) {
    *token = *input;
    *input = (XSlice){0};
    return true;
  }

  token->ptr = NULL;
  token->length = 0;
  return false;
}

X_STRING_API bool x_slice_utf8_starts_with_cstr(XSlice sv, const char* prefix)
{
  if (!sv.ptr || !prefix) return false;
  size_t prefix_len = strlen(prefix);
  if (prefix_len > sv.length) return false;
  return memcmp(sv.ptr, prefix, prefix_len) == 0;
}

X_STRING_API bool x_slice_utf8_ends_with_cstr(XSlice sv, const char* suffix)
{
  if (!sv.ptr || !suffix) return false;
  size_t suffix_len = strlen(suffix);
  if (suffix_len > sv.length) return false;
  return memcmp(sv.ptr + sv.length - suffix_len, suffix, suffix_len) == 0;
}

X_STRING_API XSlice x_slice_from_cstr(const char* s)
{
  XSlice sv;
  sv.ptr   = s ? s : "";
  sv.length = s ? (size_t)strlen(s) : 0u;
  return sv;
}

X_STRING_API XSlice x_slice_from_smallstr(const XSmallstr* s)
{
  XSlice sv;
  sv.ptr   = s ? s->buf : "";
  sv.length = s ? s->length : 0u;
  return sv;
}

X_STRING_API size_t x_smallstr_append_slice(XSmallstr* s, XSlice sv)
{
  if (!s || !sv.ptr) { return s ? s->length : 0u; }

  size_t cap   = X_SMALLSTR_MAX_LENGTH;
  size_t avail = (s->length < cap) ? (cap - s->length) : 0u;
  size_t n     = (sv.length <= avail) ? sv.length : avail;

  if (n > 0u)
  {
    memcpy(s->buf + s->length, sv.ptr, n);
    s->length += n;
  }

  s->buf[s->length] = '\0';
  return s->length;
}

X_STRING_API size_t x_smallstr_append_n(XSmallstr* s, const char* cstr, size_t n)
{
  XSlice sv;
  sv.ptr   = cstr ? cstr : "";
  sv.length = cstr ? n : 0u;
  return x_smallstr_append_slice(s, sv);
}

X_STRING_API size_t x_smallstr_vappendf(XSmallstr* s, const char* fmt, va_list args)
{
  if (!s || !fmt) { return s ? s->length : 0u; }

  size_t cap   = X_SMALLSTR_MAX_LENGTH;
  size_t avail = (s->length < cap) ? (cap - s->length) : 0u;

  if (avail == 0u)
  {
    s->buf[s->length] = '\0';
    return s->length;
  }

  /* vsnprintf writes at most avail chars plus the terminator */
  int written = vsnprintf(s->buf + s->length, avail + 1u, fmt, args);
  if (written < 0)
  {
    s->buf[s->length] = '\0';
    return s->length;
  }

  size_t used = (size_t)written;
  if (used > avail) { used = avail; } /* truncated */
  s->length += used;
  s->buf[s->length] = '\0';
  return s->length;
}

X_STRING_API size_t x_smallstr_appendf(XSmallstr* s, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  size_t r = x_smallstr_vappendf(s, fmt, args);
  va_end(args);
  return r;
}

X_STRING_API bool x_slice_contains_char(XSlice sv, char c)
{
  return x_slice_find(sv, c) >= 0;
}

X_STRING_API bool x_slice_contains_utf8(XSlice sv, uint32_t codepoint)
{
  return x_slice_utf8_find(sv, codepoint) >= 0;
}

X_STRING_API bool x_smallstr_contains_char(const XSmallstr* s, char c)
{
  XSlice sv = x_slice_from_smallstr(s);
  return x_slice_find(sv, c) >= 0;
}

X_STRING_API size_t x_smallstr_join(XSmallstr* dst, const XSlice* parts, size_t count, XSlice sep)
{
  if (!dst) { return 0u; }

  x_smallstr_clear(dst);

  for (size_t i = 0; i < count; ++i)
  {
    if (i != 0u)
    {
      x_smallstr_append_slice(dst, sep);
    }

    x_smallstr_append_slice(dst, parts[i]);

    if (dst->length == X_SMALLSTR_MAX_LENGTH)
    {
      break; /* truncated */
    }
  }

  return dst->length;
}

X_STRING_API size_t x_smallstr_capacity(void)
{
  return X_SMALLSTR_MAX_LENGTH;
}

X_STRING_API bool x_smallstr_is_empty(const XSmallstr* s)
{
  return !s || (s->length == 0u);
}

X_STRING_API bool x_smallstr_try_append_cstr(XSmallstr* s, const char* cstr, size_t* out_appended)
{
  if (out_appended) *out_appended = 0u;
  if (!s || !cstr) return false;

  size_t cap   = X_SMALLSTR_MAX_LENGTH;
  size_t avail = (s->length < cap) ? (cap - s->length) : 0u;
  if (avail == 0u) return false;

  size_t n = strlen(cstr);
  if (n == 0u) return true; /* nothing to append, but also not a failure */

  /* Copy what fits (partial allowed) */
  size_t to_copy = (n <= avail) ? n : avail;
  memcpy(s->buf + s->length, cstr, to_copy);
  s->length += to_copy;
  s->buf[s->length] = '\0';

  if (out_appended) *out_appended = to_copy;
  return to_copy > 0u;
}

#endif // X_IMPL_STRING
#endif // X_STRING_H

