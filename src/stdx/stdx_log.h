/*
 * STDX - Logging Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_LOG
 * in **one** source file before including this header.
 *
 * Notes:
 * Provides a flexible logging system with support for:
 *   - Logger initialization and cleanup
 *   - Log levels with color-coded output
 *   - Source location tagging (file, line, function)
 *   - Multiple log output targets selectable via flags
 *   - Convenience macros for common log levels (debug, info, warning, error, fatal)
 */

#ifndef X_LOG_H
#define X_LOG_H

#define X_LOG_VERSION_MAJOR 1
#define X_LOG_VERSION_MINOR 0
#define X_LOG_VERSION_PATCH 0
#define X_LOG_VERSION (X_LOG_VERSION_MAJOR * 10000 + X_LOG_VERSION_MINOR * 100 + X_LOG_VERSION_PATCH)

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
    XLOG_LEVEL_DEBUG = 0,
    XLOG_LEVEL_INFO,
    XLOG_LEVEL_WARNING,
    XLOG_LEVEL_ERROR,
    XLOG_LEVEL_FATAL,
  } XLogLevel;

  typedef enum
  {
    XLOG_OUTPUT_NONE       = 0,
    XLOG_OUTPUT_CONSOLE    = 1 << 0,
    XLOG_OUTPUT_FILE       = 1 << 1,
    XLOG_OUTPUT_BOTH       = XLOG_OUTPUT_CONSOLE | XLOG_OUTPUT_FILE,
  } XLogOutputFlags;

  typedef enum
  {
    XLOG_PLAIN      = 0,
    XLOG_TIMESTAMP  = 1 << 0,
    XLOG_TAG        = 1 << 1,
    XLOG_SOURCEINFO = 1 << 2,
    XLOG_DEFAULT    = XLOG_TAG | XLOG_TIMESTAMP | XLOG_SOURCEINFO
  } XLogComponent;

  typedef struct
  {
    FILE* file;              // Log file pointer (optional)
    int outputs;             // Which outputs enabled (console/file/both)
    XLogLevel level;          // Minimum level to log
#ifdef _WIN32
    bool vt_enabled;         // Windows VT ANSI mode enabled?
#endif
  } XLogger;

  typedef enum
  {
    XLOG_COLOR_DEFAULT,
    XLOG_COLOR_BLACK,
    XLOG_COLOR_RED,
    XLOG_COLOR_GREEN,
    XLOG_COLOR_YELLOW,
    XLOG_COLOR_BLUE,
    XLOG_COLOR_MAGENTA,
    XLOG_COLOR_CYAN,
    XLOG_COLOR_WHITE,
    XLOG_COLOR_BRIGHT_BLACK,
    XLOG_COLOR_BRIGHT_RED,
    XLOG_COLOR_BRIGHT_GREEN,
    XLOG_COLOR_BRIGHT_YELLOW,
    XLOG_COLOR_BRIGHT_BLUE,
    XLOG_COLOR_BRIGHT_MAGENTA,
    XLOG_COLOR_BRIGHT_CYAN,
    XLOG_COLOR_BRIGHT_WHITE,
  } XLogColor;

  void logger_init(XLogOutputFlags outputs, XLogLevel level, const char *filename);
  void logger_close(void);
  void logger_log(XLogLevel level, XLogColor fg, XLogColor bg, XLogComponent components, const char* file, int line, const char* func, const char* fmt,  ...);
  void logger_print(XLogLevel level, const char* fmt, ...);

#ifndef X_LOG_BREAK
#define X_LOG_BREAK() ((void)0)
#endif

#define x_log_raw(level, fg, bg, components, fmt, ...)  logger_log(level, fg, bg, components, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define x_log_debug(fmt, ...)      logger_log(XLOG_LEVEL_DEBUG,     XLOG_COLOR_BLUE,    XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*) fmt"\n", ##__VA_ARGS__)
#define x_log_info(fmt, ...)       logger_log(XLOG_LEVEL_INFO,      XLOG_COLOR_WHITE,   XLOG_COLOR_BLACK, XLOG_TIMESTAMP, __FILE__, __LINE__, __func__, (const char*) fmt"\n", ##__VA_ARGS__)
#define x_log_warning(fmt, ...)    logger_log(XLOG_LEVEL_WARNING,   XLOG_COLOR_YELLOW,  XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*) fmt"\n", ##__VA_ARGS__)
#define x_log_error(fmt, ...)      logger_log(XLOG_LEVEL_ERROR,     XLOG_COLOR_RED,     XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*) fmt"\n", ##__VA_ARGS__)
#define x_log_fatal(fmt, ...)      do{ logger_log(XLOG_LEVEL_FATAL, XLOG_COLOR_WHITE,   XLOG_COLOR_RED,   XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*) fmt"\n", ##__VA_ARGS__); X_LOG_BREAK();} while(0);


#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_LOG

#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

  static XLogger g_logger =
  {
    .file = NULL,
    .outputs = XLOG_OUTPUT_CONSOLE,
    .level = XLOG_LEVEL_DEBUG,
#ifdef _WIN32
    .vt_enabled = false,
#endif
  };

  /* Internal helpers */

  static int map_color_to_ansi(XLogColor color, bool fg)
  {
    (void)(fg);

    const int ANSI_BACKGROUND = 10;
    switch (color) {
      case XLOG_COLOR_BLACK:          return 30 + ANSI_BACKGROUND;
      case XLOG_COLOR_RED:            return 31 + ANSI_BACKGROUND;
      case XLOG_COLOR_GREEN:          return 32 + ANSI_BACKGROUND;
      case XLOG_COLOR_YELLOW:         return 33 + ANSI_BACKGROUND;
      case XLOG_COLOR_BLUE:           return 34 + ANSI_BACKGROUND;
      case XLOG_COLOR_MAGENTA:        return 35 + ANSI_BACKGROUND;
      case XLOG_COLOR_CYAN:           return 36 + ANSI_BACKGROUND;
      case XLOG_COLOR_WHITE:          return 37 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_BLACK:   return 90 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_RED:     return 91 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_GREEN:   return 92 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_YELLOW:  return 93 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_BLUE:    return 94 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_MAGENTA: return 95 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_CYAN:    return 96 + ANSI_BACKGROUND;
      case XLOG_COLOR_BRIGHT_WHITE:   return 97 + ANSI_BACKGROUND;
      default:                   return 39 + ANSI_BACKGROUND; // reset to default
    }
  }

  /* Enable VT processing on Windows 10+ */
  static inline void enable_windows_vt(void)
  {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hOut, dwMode))
    {
      g_logger.vt_enabled = true;
    }
  }

  /* Windows console API color codes for fallback */
  static inline WORD win_console_color(XLogLevel level)
  {
    switch (level)
    {
      case XLOG_LEVEL_DEBUG: return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_INFO: return 15 | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_WARNING: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_ERROR: return FOREGROUND_RED | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_FATAL: return BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_INTENSITY;
      default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
  }

  static inline WORD map_color_to_win_attr(XLogColor color, bool is_fg)
  {
    switch (color)
    {
      case XLOG_COLOR_BLACK:          return is_fg ? 0 : 0;
      case XLOG_COLOR_RED:            return is_fg ? FOREGROUND_RED : BACKGROUND_RED;
      case XLOG_COLOR_GREEN:          return is_fg ? FOREGROUND_GREEN : BACKGROUND_GREEN;
      case XLOG_COLOR_YELLOW:         return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN : BACKGROUND_RED | BACKGROUND_GREEN;
      case XLOG_COLOR_BLUE:           return is_fg ? FOREGROUND_BLUE : BACKGROUND_BLUE;
      case XLOG_COLOR_MAGENTA:        return is_fg ? FOREGROUND_RED | FOREGROUND_BLUE : BACKGROUND_RED | BACKGROUND_BLUE;
      case XLOG_COLOR_CYAN:           return is_fg ? FOREGROUND_GREEN | FOREGROUND_BLUE : BACKGROUND_GREEN | BACKGROUND_BLUE;
      case XLOG_COLOR_WHITE:          return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
      case XLOG_COLOR_BRIGHT_BLACK:   return is_fg ? FOREGROUND_INTENSITY : 0;
      case XLOG_COLOR_BRIGHT_RED:     return is_fg ? FOREGROUND_RED | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_GREEN:   return is_fg ? FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_YELLOW:  return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_BLUE:    return is_fg ? FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_MAGENTA: return is_fg ? FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_CYAN:    return is_fg ? FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_WHITE:   return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
                                      : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      default: return 0;
    }
  }

  /* Output message with Windows Console API colors */
  static inline void x_log_output_console_winapi(XLogColor fg, XLogColor bg, const char* msg)
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
    {
      fputs(msg, stdout);
      return;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
      fputs(msg, stdout);
      return;
    }
    WORD oldAttrs = csbi.wAttributes;
    WORD color = map_color_to_win_attr(fg, true)|map_color_to_win_attr(bg, false);
    SetConsoleTextAttribute(hConsole, color);
    fputs(msg, stdout);
    SetConsoleTextAttribute(hConsole, oldAttrs);
  }

#else // _WIN32

  /* Output message with ANSI colors */
  static inline void x_log_output_console_ansi(XLogColor fg, XLogColor bg const char *msg)
  {
    //const char* color = ansi_color_code(level);
    char* color = "\x1b[%d;%dm";
    char color[32];
    snprintf(color, sizeof(color),
        "\x1b[%d;%dm",
        map_color_to_ansi(fg, true),
        map_color_to_ansi(bg, false));
    fprintf(stdout, "%s%s\x1b[0m", color, msg);
  }
#endif

  /* Common console output */
  static inline void x_log_output_console(XLogColor fg, XLogColor bg, const char *msg)
  {
#ifdef _WIN32
    //const char* color_code = ansi_color_code(level);


    if (g_logger.vt_enabled)
    {
      /* Use ANSI */
      fprintf(stdout,
          "\x1b[%d;%dm%s\x1b[0m",
          map_color_to_ansi(fg, true),
          map_color_to_ansi(bg, false),
          msg);
    }
    else
    {
      /* Use Windows Console API */
      x_log_output_console_winapi(fg, bg, msg);
    }
#else
    x_log_output_console_ansi(msg, level);
#endif
  }

  /* File output (no colors) */
  static inline void x_log_output_file(const char *msg)
  {
    if (g_logger.file)
    {
      fputs(msg, g_logger.file);
      fflush(g_logger.file);
    }
  }

  void logger_log(XLogLevel level, XLogColor fg, XLogColor bg, XLogComponent components, const char* file, int line, const char* func, const char* fmt, ...)
  {
    static const char* x_log_level_strings[] =
    {
      "DEBUG",
      "INFO",
      "WARNING",
      "ERROR",
      "FATAL"
    };

    if (level < g_logger.level)
      return;

    char timebuf[30] = {0};
    if (components & XLOG_TIMESTAMP)
    {
      time_t t = time(NULL);
      struct tm tm_info;

#ifdef _WIN32
      localtime_s(&tm_info, &t);
#else
      localtime_r(&t, &tm_info);
#endif
      strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S] ", &tm_info);
    }

    char tag[32] = {0};
    if (components & XLOG_TAG)
    {
      snprintf((char*) tag, sizeof(tag), "%s ", x_log_level_strings[level]);
    }

    char source_info[1024] = {0};
    if (components & XLOG_SOURCEINFO)
    {
      snprintf(source_info, sizeof(source_info), "%s:%d %s() : ", file, line, func);
    }

    /* Format the message body */
    char msgbuf[1024];
    va_list args;
    va_start(args, fmt);

#ifdef _WIN32
    vsnprintf_s(msgbuf, sizeof(msgbuf), _TRUNCATE, fmt, args);
#else
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
#endif
    va_end(args);


    char finalbuf[1280];
    snprintf(finalbuf, sizeof(finalbuf), "%s%s%s%s",
        tag,
        timebuf,
        source_info, msgbuf);

    if (g_logger.outputs & XLOG_OUTPUT_CONSOLE)
    {
      x_log_output_console(fg, bg, finalbuf);
    }

    if (g_logger.outputs & XLOG_OUTPUT_FILE)
    {
      x_log_output_file(finalbuf);
    }
  }

  /* Initialize logger */
  void logger_init(XLogOutputFlags outputs, XLogLevel level, const char *filename)
  {
    g_logger.outputs = outputs;
    g_logger.level = level;
    printf("log initialized\n");

#ifdef _WIN32
    enable_windows_vt();
#endif

    if ((outputs & XLOG_OUTPUT_FILE) && filename != NULL)
    {
      g_logger.file = fopen(filename, "a");
      if (!g_logger.file)
      {
        fprintf(stderr, "ERROR: Failed to open log file '%s'\n", filename);
        g_logger.outputs &= ~XLOG_OUTPUT_FILE; /* disable file output */
      }
    }
  }

  /* Close logger and free resources */
  void logger_close(void)
  {
    if (g_logger.file)
    {
      fclose(g_logger.file);
      g_logger.file = NULL;
    }
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_LOG
#endif // X_LOG_H

