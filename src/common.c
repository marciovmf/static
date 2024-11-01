#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Logging functions */

void debug_print(void* stream, const char* prefix, const char* fmt, ...)
{
  va_list arg_list;
  va_start(arg_list, fmt);
  fprintf(stream, "%s\t", prefix);
  vfprintf(stream, fmt, arg_list);
  va_end(arg_list);
}


void debug_print_detailed(void* stream, const char* prefix, int line, const char* file, const char* function, const char* fmt, ...)
{
  va_list arg_list;
  va_start(arg_list, fmt);
  fprintf(stream, (const char*) "%s\t(%s @ %s:%d) - ", prefix, function, file, line);
  vfprintf(stream, fmt, arg_list);
  va_end(arg_list);
}


/* String utility functions */

bool str_ends_with(const char* str, const char* suffix)
{
  const char* found = strstr(str, suffix);
  if (found == NULL || found + strlen(suffix) != str + strlen(str))
  {
    return false;
  }
  return true;
}


bool str_starts_with(const char* str, const char* prefix)
{
  const char* found = strstr(str, prefix);
  return found == str;
}


size_t smallstr(Smallstr* smallString, const char* str)
{
  size_t length = strlen(str);
  if (length >= SMALLSTR_MAX_LENGTH)
  {
    log_error("The string length (%d) exceeds the maximum size of a LDKSmallString (%d)", length, SMALLSTR_MAX_LENGTH);
    return length;
  }

  strncpy((char *) &smallString->str, str, SMALLSTR_MAX_LENGTH - 1);
  return 0;
}


size_t smallstr_format(Smallstr* smallString, const char* fmt, ...)
{
  va_list argList;
  va_start(argList, fmt);
  smallString->length = vsnprintf((char*) &smallString->str, SMALLSTR_MAX_LENGTH-1, fmt, argList);
  va_end(argList);

  bool success = smallString->length > 0 && smallString->length < SMALLSTR_MAX_LENGTH;
  return success;
}


size_t smallstr_from_subsring(Substr* substring, Smallstr* outSmallString)
{
  if (substring->length >= (SMALLSTR_MAX_LENGTH - 1))
    return substring->length;

  strncpy((char*) &outSmallString->str, substring->ptr, substring->length);
  outSmallString->str[substring->length] = 0;
  return 0;
}


size_t smallstr_length(Smallstr* smallString)
{
  return smallString->length;
}


void smallstr_clear(Smallstr* smallString)
{
  memset(smallString->str, 0, SMALLSTR_MAX_LENGTH * sizeof(char));
}

/* File reading */


int read_entire_file_to_memory(const char *file_name, char **out_buffer, size_t *out_file_size, bool null_terminate)
{
  size_t file_size;
  FILE *file = fopen(file_name, "rb");
  if (!file)
  {
    log_error("Failed to open file");
    return -1; // Return -1 on error
  }

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);


  *out_buffer = (char *) malloc(file_size + (null_terminate ? 1 : 0) );
  if (*out_buffer == NULL)
  {
    fclose(file);
    perror("Failed to allocate memory");
    return -1;
  }

  size_t bytesRead = fread(*out_buffer, 1, file_size, file);
  fclose(file);

  if (bytesRead != file_size)
  {
    free(*out_buffer);
    *out_buffer = NULL;
    log_error("Failed to read complete file %d of %d", bytesRead, file_size);
    return -1;
  }

  if (null_terminate)
    (*out_buffer)[bytesRead] = 0;

  if (out_file_size)
    *out_file_size = file_size;
  return 0;
}

char* strdup_safe(const char* str)
{
  if (str == NULL)
    return NULL;
  char* copy = (char*)malloc(strlen(str) + 1);
  if (copy) strcpy(copy, str);
  return copy;
}
