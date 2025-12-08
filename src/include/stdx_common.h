/*
 * STDX - Common
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * Provides portable macros and types for different taks such as os detection,
 * architecture detection, compiler detection, bit manipulation, dll
 * export/import macros, assertions and more.
 */

#ifndef X_COMMON_H
#define X_COMMON_H

#ifdef __cplusplus
extern "C" {
#include <stdbool.h>
#endif


#define X_COMMON_VERSION_MAJOR 1
#define X_COMMON_VERSION_MINOR 0
#define X_COMMON_VERSION_PATCH 0

#define X_COMMON_VERSION (X_COMMON_VERSION_MAJOR * 10000 + X_COMMON_VERSION_MINOR * 100 + X_COMMON_VERSION_PATCH)

  // -----------------------------------------------------------------------------
  // Compiler Detection macros
  // -----------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__GNUG__)
#define X_COMPILER_NAME "GCC"
#define X_COMPILER_VERSION __VERSION__
#define X_COMPILER_GCC
#elif defined(_MSC_VER)
#define X_COMPILER_NAME "Microsoft Visual C/C++ Compiler"
#define X_COMPILER_VERSION _MSC_FULL_VER
#define X_COMPILER_MSVC
#elif defined(__clang__)
#define X_COMPILER_NAME "Clang"
#define X_COMPILER_VERSION __clang_version__
#define X_COMPILER_CLANG
#else
#define X_COMPILER_NAME "Unknown"
#define X_COMPILER_VERSION "Unknown"
#define X_COMPILER_UNKNOWN
#endif

  // -----------------------------------------------------------------------------
  //  Compiler ATTRIBUTES / INLINE / ALIGN
  // -----------------------------------------------------------------------------

#if defined(COMPILER_MSVC)
#define X_INLINE      __inline
#define X_FORCEINLINE __forceinline
#define X_NOINLINE    __declspec(noinline)
#define X_NORETURN    __declspec(noreturn)
#define X_ALIGN(n)    __declspec(align(n))
#elif defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
#define X_INLINE      inline
#define X_FORCEINLINE inline __attribute__((always_inline))
#define X_NOINLINE    __attribute__((noinline))
#define X_NORETURN    __attribute__((noreturn))
#define X_ALIGN(n)    __attribute__((aligned(n)))
#else
#define X_INLINE
#define X_FORCEINLINE
#define X_NOINLINE
#define X_NORETURN
#define X_ALIGN(n)
#endif


  // ----------------------------------------------------------------------------
  // Branch prediction
  // ----------------------------------------------------------------------------

#if defined(X_PLAT_COMPILER_GCC) || defined(X_PLAT_COMPILER_CLANG)
#define X_LIKELY(x)   __builtin_expect(!!(x), 1)
#define X_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define X_LIKELY(x)   (x)
#define X_UNLIKELY(x) (x)
#endif


  // ----------------------------------------------------------------------------
  // Architecture detection
  // ----------------------------------------------------------------------------

#if defined(_M_X64) || defined(__x86_64__)
#define X_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define X_ARCH_X86 1
#elif defined(__aarch64__)
#define X_ARCH_ARM64 1
#elif defined(__arm__)
#define X_ARCH_ARM32 1
#else
#define X_ARCH_UNKNOWN 1
#endif


  // -----------------------------------------------------------------------------
  // OS Detection macros
  // -----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#define X_OS_WINDOWS 1
#elif defined(__linux__)
#define X_OS_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
#define X_OS_OSX
#else
#define X_OS_UNKNOWN
#endif


  // ----------------------------------------------------------------------------
  // DLL import/export
  // ----------------------------------------------------------------------------

#if defined(X_OS_WINDOWS)
#define X_PLAT_EXPORT __declspec(dllexport)
#define X_PLAT_IMPORT __declspec(dllimport)
#else
#define PLAT_EXPORT __attribute__((visibility("default")))
#define PLAT_IMPORT
#endif


#if defined(_DEBUG)
#ifndef DEBUG
#define DEBUG
#endif
#endif


  // ----------------------------------------------------------------------------
  // Assertion macros
  // ----------------------------------------------------------------------------

#define X_STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond) ? 1 : -1]



#ifdef DEBUG
#define X_ASSERT(expr) \
  do { \
    if (!(expr)) { \
      fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
      abort(); \
    } \
  } while(0)
#else
#define X_ASSERT(expr) ((void)0)
#endif

  // ----------------------------------------------------------------------------
  // Endianness
  // ----------------------------------------------------------------------------

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define X_BIG_ENDIAN 1
#else
#define X_LITTLE_ENDIAN 1
#endif

  // ----------------------------------------------------------------------------
  // Path separator
  // ----------------------------------------------------------------------------

#if defined(X_OS_WINDOWS)
#define X_PATH_SEPARATOR              '\\'
#define X_PATH_SEPARATOR_ALTERNATIVE  '/'
#else
#define X_PATH_SEPARATOR              '/'
#define X_PATH_SEPARATOR_ALTERNATIVE  '\\'
#endif


  // -----------------------------------------------------------------------------
  // Bit manipulation macros
  // -----------------------------------------------------------------------------

#define X_BIT_SET(var, bit)    ((var) |=  (1U << (bit)))
#define X_BIT_CLEAR(var, bit)  ((var) &= ~(1U << (bit)))
#define X_BIT_TOGGLE(var, bit) ((var) ^=  (1U << (bit)))
#define X_BIT_CHECK(var, bit)  (((var) &  (1U << (bit))) != 0)
#define X_UNUSED(x) (void)(x) // mark a function argument as unused


  // -----------------------------------------------------------------------------
  // Wraps a value of a primitive type (e.g., int, float, etc.) in a temporary
  // compound literal and returns a pointer to it.
  //  - Requires C99 or later (for compound literals).
  //  - The pointer must not be stored or used after the function call.
  // -----------------------------------------------------------------------------

#define X_VALUE_PTR(type, val) ((const void*)&(type){ (val) })
#define X_VALUE_TYPE_PTR(type, val) ((const type*)&(type){ (val) })


  // -----------------------------------------------------------------------------
  // A XPtr holds either a valid pointer or an error code.
  // Used for functions that return a pointers but may fail.
  // -----------------------------------------------------------------------------
  
  typedef struct XPtr_t XPtr;
  struct XPtr_t
  {
    int error;
    void *ptr;
  };

#define X_PTR_OK(p)      ((XPtr){ .error = 0, .ptr = (p) })
#define X_PTR_ERR(e)     ((XPtr){ .error = (e), .ptr = 0 })
#define X_PTR_IS_ERR(r)  ((r).error != 0)
#define X_PTR_IS_OK(r)   ((r).error == 0)
#define X_PTR_GET_PTR(r) ((r).is_error ? NULL : (r).ptr)
#define X_PTR_GET_ERR(r) ((r).error)
#define X_PTR_CAST(r, T) ((r).is_error ? NULL : (T)(r).ptr)


  // -----------------------------------------------------------------------------
  // Common types
  // -----------------------------------------------------------------------------

#if defined(X_OS_WINDOWS)
#if defined (X_COMPILER_MSVC)
#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif 

#endif // X_OS_WINDOWS

#include <stdint.h>
#if defined(_DEBUG) || defined(DEBUG)
#include <stdio.h>
#endif

  typedef int8_t    i8;
  typedef uint8_t   u8;
  typedef uint8_t   byte;
  typedef int16_t   i16;
  typedef uint16_t  u16;
  typedef int32_t   i32;
  typedef uint32_t  u32;
  typedef int64_t   i64;
  typedef uint64_t  u64;

  typedef struct XSize_t { i32 w, h; }  XSize;
  typedef struct XPoint_t { i32 x, y; } XPoint;

#ifndef X_ARRAY_COUNT
#define X_ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#endif

#ifdef __cplusplus
}
#endif

#endif //X_COMMON_H

