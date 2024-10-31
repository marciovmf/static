/**
 * @file common.h
 * @brief Common Definitions and Utilities
 *
 * This header provides essential types, macros, and utility functions for
 * managing strings, logging, and assertions across different platforms and 
 * compilers. It includes:
 * - OS and compiler detection macros
 * - Logging functions for different severity levels
 * - String utilities for handling small strings and substrings
 * - Assertion macros for debugging
 *
 * @author marciovmf
 * @version 1.0
 * @date 2024-10-29
 */

#ifndef COMMON_H
#define COMMON_H

#ifndef __cplusplus
#include <stdbool.h>
#endif  


#ifdef __cplusplus
extern "C" {
#endif


  /* OS Detection macros */

#if defined(_WIN32) || defined(_WIN64)
# define OS_WINDOWS
#elif defined(__linux__)
# define OS_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
# define OS_OSX
#else
# define OS_UNKNOWN
#endif

  /* compiler detection macros */

#if defined(__GNUC__) || defined(__GNUG__)
# define COMPILER_NAME "GCC"
# define COMPILER_VERSION __VERSION__
# define COMPILER_GCC
#elif defined(_MSC_VER)
# define COMPILER_NAME "Microsoft Visual C/C++ Compiler"
# define COMPILER_VERSION _MSC_FULL_VER
# define COMPILER_MSVC
#elif defined(__clang__)
# define COMPILER_NAME "Clang"
# define COMPILER_VERSION __clang_version__
# define COMPILER_CLANG
#else
# define COMPILER_NAME "Unknown"
# define COMPILER_VERSION "Unknown"
# define COMPILER_UNKNOWN
#endif

#if defined(DEBUG) || defined(_DEBUG)
# ifndef DEBUG
#   define DEBUG
# endif
#endif

#if defined(OS_WINDOWS)
# define _CRT_SECURE_NO_WARNINGS
#endif

  /* logging macros */

#ifdef COMPILER_MSVC
# define log_info(fmt, ...) debug_print(stdout, "INFO",  fmt, __VA_ARGS__)
# define log_error(fmt, ...) debug_print_detailed(stderr, "ERROR", __LINE__, __FILE__, __func__, fmt, __VA_ARGS__)
# define log_warning(fmt, ...) debug_print(stdout, "WARNING", fmt, __VA_ARGS__)
# ifdef DEBUG
#   define log_debug(fmt, ...) ldkLogPrintDetailed(stdout, "DEBUG", __FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
# else
#   define log_debug(fmt, ...)
# endif //DEBUG
# else
# define log_info(fmt, ...) debug_print(stdout, "INFO",  fmt, ##__VA_ARGS__)
# define log_error(fmt, ...) debug_print_detailed(stderr, "ERROR", __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define log_warning(fmt, ...) debug_print_detailed(stdout, "WARNING", __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#     ifdef DEBUG
#       define log_warning(fmt, ...) debug_print_detailed(stdout, "WARNING", __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#     else
#       define log_debug(stdout, fmt, ...)
#     endif //DEBUG
#endif //COMPILER_MSVC

# define log_and_break(fmt, ...) do {debug_print_detailed(stderr, "FATAL", __LINE__, __FILE__, __func__, fmt, __VA_ARGS__); ASSERT_BREAK(); } while(0);


  /* Assertion macros */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#define ASSERT_BREAK() (*(volatile int*)0 = 0)
#define ASSERT(CONDITION) do{\
  if (! (CONDITION)) {\
    printf("ASSERTION FAILED AT %s:%d\n", __FILE__, __LINE__);\
    ASSERT_BREAK();\
  }}while(0)


#define ASSERT_NOT_IMPLEMENTED() do{ log_error("Not implemented"); ASSERT_BREAK();} while(0)


  /* Utility macros */

  #define UNUSED(x) (void)(x) // mark a function argument as unused

  /* Common types */

#include <stdint.h>
  typedef int8_t    i8;
  typedef uint8_t   u8;
  typedef uint8_t   byte;
  typedef int16_t   i16;
  typedef uint16_t  u16;
  typedef int32_t   i32;
  typedef uint32_t  u32;
  typedef int64_t   i64;
  typedef uint64_t  u64;


  /* Logging functions */

  void debug_print(void* stream, const char* prefix, const char* fmt, ...);
  void debug_print_detailed(void* stream, const char* prefix, int line, const char* file, const char* function, const char* fmt, ...);


  /* String utilities */

#ifndef SMALLSTR_MAX_LENGTH
# define SMALLSTR_MAX_LENGTH 128
#endif

  typedef struct
  {
    char* ptr;
    size_t length;
  } Substr;

  typedef struct
  {
    char str[SMALLSTR_MAX_LENGTH];
    size_t length;
  } Smallstr;


  /* String utility functions */

  /**
   * @brief Checks if a given string ends with a specified suffix.
   * 
   * @param str The main string to check.
   * @param suffix The suffix to look for at the end of the main string.
   * @return true if the main string ends with the specified suffix, false otherwise.
   */
  bool str_ends_with(const char* str, const char* suffix);

  /**
   * @brief Checks if a given string starts with a specified prefix.
   * 
   * @param str The main string to check.
   * @param prefix The prefix to look for at the beginning of the main string.
   * @return true if the main string starts with the specified prefix, false otherwise.
   */
  bool str_starts_with(const char* str, const char* prefix);

  /**
   * @brief Copies a string into an Smallstr structure, truncating if necessary.
   * 
   * @param smallString The Smallstr structure to store the copied string.
   * @param str The source string to copy.
   * @return The length of the copied string, up to SMALLSTR_MAX_LENGTH.
   */
  size_t smallstr(Smallstr* smallString, const char* str);

  /**
   * @brief Formats a string according to a format specifier and stores it in an Smallstr.
   * 
   * @param smallString The Smallstr structure to store the formatted string.
   * @param fmt The format specifier (printf-style).
   * @param ... Additional arguments for the format specifier.
   * @return The length of the formatted string, up to SMALLSTR_MAX_LENGTH.
   */
  size_t smallstr_format(Smallstr* smallString, const char* fmt, ...);

  /**
   * @brief Copies a substring into an Smallstr structure.
   * 
   * @param substring The Substr containing the substring to copy.
   * @param outSmallString The Smallstr structure to store the copied substring.
   * @return The length of the copied substring.
   */
  size_t smallstr_from_subsring(Substr* substring, Smallstr* outSmallString);

  /**
   * @brief Retrieves the length of a string stored in an Smallstr.
   * 
   * @param smallString The Smallstr structure to check.
   * @return The length of the string.
   */
  size_t smallstr_length(Smallstr* smallString);

  /**
   * @brief Clears the contents of an Smallstr, setting its length to zero.
   * 
   * @param smallString The Smallstr structure to clear.
   */
  void smallstr_clear(Smallstr* smallString);



  /* File reading */

/**
 * Reads the entire contents of a specified file into a dynamically allocated memory buffer.
 *
 * @param file_name   A pointer to a null-terminated string containing the name of the file to be read.
 * @param out_buffer  A double pointer to a character buffer. This will be allocated within the function 
 *               and will point to the buffer containing the file contents upon success.
 * @param out_file_size A pointer to a size_t that will be updated with the size of the file in bytes upon successful completion of the function.
 * @param null_terminate A boolean indicating if the buffer should be null terminated. If true an extra byte will be allocated for the null terminator.
 * 
 * @return 0 on success, or -1 if an error occurs (e.g., if the file cannot be opened, memory 
 *         allocation fails, or the file cannot be read completely).
 * 
 * @Note: The memory allocated for the buffer must be freed by the caller using the free().
 */
  int read_entire_file_to_memory(const char *file_name, char **out_buffer, size_t *out_file_size, bool null_terminate);



  /*
   * Utility function to duplicate strings
   */
  char* strdup_safe(const char* str);

#ifdef __cplusplus
}
#endif

#endif //COMMON_H

