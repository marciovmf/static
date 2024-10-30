#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "filesystem.h"

#ifdef _WIN32
# include <windows.h>
#else
# include <dirent.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

size_t fs_cwd_get(FSPath* path)
{
  DWORD size = GetCurrentDirectory(FS_PATH_MAX_LENGTH, path->path);
  return (size_t)size;
}


size_t fs_cwd_set(const char* path)
{
  return SetCurrentDirectory(path) != 0;
}


size_t fs_path_from_executable(FSPath* out)
{
  size_t len = GetModuleFileNameA(NULL, (char*) &out->path, FS_PATH_MAX_LENGTH);
  char* end = out->path + len;

  while (end > (char*) &out->path && *end != '\\'  && *end != '/' )
  {
    *end = 0;
    len--;
    end--;
  }

  out->length = len;
  return len;
}


size_t fs_cwd_set_from_executable_path(void)
{
  FSPath program_path;
  size_t bytesCopied = fs_path_from_executable(&program_path);
  fs_cwd_set(program_path.path);
  return bytesCopied;
}


bool fs_file_copy(const char* file, const char* newFile)
{
  return CopyFile(file, newFile, FALSE) != 0;
}


bool fs_file_rename(const char* file, const char* newFile)
{
  return MoveFile(file, newFile) != 0;
}


bool fs_directory_create(const char* path)
{
  return CreateDirectory(path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}


bool fs_directory_create_recursive(const char* path)
{
  size_t length = strlen(path);
  if (length >= FS_PATH_MAX_LENGTH)
  {
    log_error("FSPath is too long (%d). Maximum size is %d. - '%s'", length, FS_PATH_MAX_LENGTH, path);
    return false;
  }

  char temp_path[FS_PATH_MAX_LENGTH];
  strncpy((char *) &temp_path, path, length);
  temp_path[length] = 0;

  char* p = (char*) &temp_path;

  // If the path is absolute, skip the drive letter and colon
  if (p[1] == ':') { p += 2; }

  while (*p)
  {
    if (*p == '\\' || *p == '/')
    {
      *p = '\0';
      if (!CreateDirectory((const char*) temp_path, NULL))
      {
        if (GetLastError() != ERROR_ALREADY_EXISTS) { return false; }
      }
      *p = '\\';
    }
    p++;
  }

  return true;
}


bool fs_directory_delete(const char* directory)
{
  return RemoveDirectory(directory) != 0;
}


bool fs_path_is_file(const char* path)
{
  DWORD attributes = GetFileAttributes(path);
  return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}


bool fs_path_is_directory(const char* path)
{
  DWORD attributes = GetFileAttributes(path);
  return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}


bool fs_path_exists(const char* path)
{
  DWORD attributes = GetFileAttributes(path);
  return (attributes != INVALID_FILE_ATTRIBUTES);
}


Substr fs_path_get_file_name(const char* path)
{
  Substr substr = {0};
  substr.ptr = NULL;
  substr.length = 0;

  if (path)
  {
    const char* dot = strrchr(path, '.');
    if (dot)
    {
      substr.ptr = (char*)(dot + 1);
      substr.length = strlen(substr.ptr);
    }
  }
  return substr;
}


Substr fs_path_get_file_extension(const char* path)
{
  Substr substr = {0};
  substr.ptr = NULL;
  substr.length = 0;

  if (path)
  {
    const char* dot = strrchr(path, '.');
    if (dot)
    {
      substr.ptr = (char*)(dot + 1);
      substr.length = strlen(substr.ptr);
    }
  }

  return substr;
}


Substr fs_path_get_parent(const char* path)
{
  Substr parent = {0};
  parent.ptr = NULL;
  parent.length = 0;

  if (path == NULL)
    return parent;

  const char* last_separator = strrchr(path, '/'); // For UNIX-style paths
  if (!last_separator)
  {
    last_separator = strrchr(path, '\\'); // For Windows-style paths
  }

  if (last_separator != NULL)
  {
    size_t parent_length = last_separator - path;
    strncpy(parent.ptr, path, parent_length);
    parent.ptr[parent_length] = '\0';
    parent.length = parent_length;
  }

  return parent;
}


bool fs_path_is_absolute(const char* path)
{
#ifdef _WIN32
  const char* p = path;
  size_t length = strlen(path);
  if (length >= 3)
    return (isalpha(p[0]) && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) || (p[0] == '\\' && p[1] == '\\');
  else
    return (isalpha(p[0]) && p[1] == ':') || (p[0] == '\\' && p[1] == '\\');
#else
  return path[0] == '/';
#endif
}


bool fs_path_is_relative(const char* path)
{
  return ! fs_path_is_absolute(path);
}


void fs_path_normalize(FSPath* path)
{
  char *p = path->path;
  size_t len = path->length;
  size_t result_index = 0;
  size_t i = 0;


  if (p[1] == ':') { i += 2; }

  // Skip leading slashes
  while (i < len && (p[i] == '/' || p[i] == '\\'))
  {
    i++;
  }

  for (; i < len; i++) {
    if (p[i] == '/' || p[i] == '\\')
    {
      // Skip multiple consecutive slashes
      while (i < len && (p[i] == '/' || p[i] == '\\')) {
        i++;
      }
      // Add a single slash to the result
      path->path[result_index++] = '/';
    } else if (p[i] == '.' && (i + 1 == len || p[i + 1] == '/' || p[i + 1] == '\\')) {
      // Skip "." or "./"
      i++;
    } else if (p[i] == '.' && p[i + 1] == '.' && (i + 2 == len || p[i + 2] == '/' || p[i + 2] == '\\')) {
      // Handle up-level reference ".."
      if (result_index > 0) {
        // Remove the last component from the result
        while (result_index > 0 && path->path[result_index - 1] != '/') {
          result_index--;
        }
        if (result_index > 0) {
          result_index--;
        }
      }
      i += 2;
    } else {
      // Copy characters to the result
      path->path[result_index++] = p[i];
    }
  }

  // Update the length and null-terminate the result
  path->length = result_index;
  path->path[result_index] = '\0';
}


bool fs_path(FSPath* out, const char* path)
{
  size_t length = strlen(path);
  if (length >= (FS_PATH_MAX_LENGTH - 1))
  {
    log_error("FSPath too long (%d). Maximum is %d. - '%s'", length, FS_PATH_MAX_LENGTH, path);
    return false;
  }

  strncpy((char*) &out->path, path, length);
  out->path[length] = 0;
  out->length = length;
  return true;
}


void fs_path_clone(FSPath* out, const FSPath* path)
{
  memcpy(out, path, sizeof(FSPath));
}


// directory listing

// Structure to hold directory search state
struct FSDirectoryHandle_t
{
#ifdef _WIN32
    HANDLE handle;
    WIN32_FIND_DATA findData;
#else
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;
#endif
}; 


FSDirectoryHandle* fs_find_first_file(const char* path, FSDirectoryEntry* entry)
{
  FSDirectoryHandle* dir_handle = malloc(sizeof(FSDirectoryHandle));
  if (!dir_handle) return NULL;

#ifdef _WIN32
  char searchPath[MAX_PATH];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

  dir_handle->handle = FindFirstFile(searchPath, &dir_handle->findData);
  if (dir_handle->handle == INVALID_HANDLE_VALUE)
  {
    free(dir_handle);
    return NULL;
  }

  // Fill DirectoryEntry with the first entry
  strncpy(entry->name, dir_handle->findData.cFileName, MAX_PATH);
  entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
  entry->last_modified = *(time_t*)&dir_handle->findData.ftLastWriteTime;
  entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#else
  dir_handle->dir = opendir(path);
  if (!dir_handle->dir)
  {
    free(dir_handle);
    return NULL;
  }

  // Read the first entry and fill DirectoryEntry
  dir_handle->entry = readdir(dir_handle->dir);
  if (dir_handle->entry)
  {
    snprintf(entry->name, MAX_PATH, "%s", dir_handle->entry->d_name);

    // Get file stats to fill size and last modified time
    char fullPath[MAX_PATH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->name);
    stat(fullPath, &dir_handle->fileStat);
    entry->size = dir_handle->fileStat.st_size;
    entry->last_modified = dir_handle->fileStat.st_mtime;
    entry->is_directory = (dir_handle->entry->d_type == DT_DIR);
  }
#endif

  return dir_handle;
}


int fs_find_next_file(FSDirectoryHandle* dir_handle, FSDirectoryEntry* entry)
{
#ifdef _WIN32
  if (FindNextFile(dir_handle->handle, &dir_handle->findData))
  {
    strncpy(entry->name, dir_handle->findData.cFileName, MAX_PATH);
    entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
    entry->last_modified = *(time_t*)&dir_handle->findData.ftLastWriteTime; // Convert to time_t
    entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return 1;
  }
#else
  dir_handle->entry = readdir(dir_handle->dir);
  if (dir_handle->entry)
  {
    snprintf(entry->name, MAX_PATH, "%s", dir_handle->entry->d_name);

    // Get file stats to fill size and last modified time
    char fullPath[MAX_PATH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", dir_handle->dir, entry->name);
    stat(fullPath, &dir_handle->fileStat);
    entry->size = dir_handle->fileStat.st_size;
    entry->last_modified = dir_handle->fileStat.st_mtime;
    entry->is_directory = (dir_handle->entry->d_type == DT_DIR);
    return 1;
  }
#endif
  return 0;
}


void fs_find_close(FSDirectoryHandle* dir_handle)
{
#ifdef _WIN32
  FindClose(dir_handle->handle);
#else
  closedir(dir_handle->dir);
#endif
  free(dir_handle);
}

int fs_get_temp_folder(FSPath* out)
{

#ifdef _WIN32
  // On Windows, use GetTempPath to retrieve the temporary folder path
  DWORD path_len = GetTempPathA((DWORD) FS_PATH_MAX_LENGTH, out->path);
  if (path_len == 0 || path_len > FS_PATH_MAX_LENGTH)
  {
    log_error("Failed to get temp folder path", 0);
    return -1;
  }
  out->length = path_len;
#else
  // On Unix-based systems, check environment variables for temporary folder path
  const char *tmp_dir = getenv("TMPDIR");
  if (!tmp_dir) tmp_dir = getenv("TEMP");
  if (!tmp_dir) tmp_dir = getenv("TMP");
  if (!tmp_dir) tmp_dir = "/tmp"; // Default to /tmp if no environment variable is set

  // Copy the temporary folder path to buffer
  if (strlen(tmp_dir) >= FS_PATH_MAX_LENGTH)
  {
    log_error("Buffer size too small for temp folder path\n", 0);
    return -1;
  }
  fs_path(out, tmp_dir);
#endif

  return 0;
}
