/**
 * @file filesystem.h
 * @brief Interface for File System Operations
 *
 * This header provides functionality for managing files and directories, 
 * obtaining the current working directory, and manipulating file paths. 
 * It defines structures, constants, and function prototypes for tasks such as:
 * - Managing current working directories
 * - Copying, renaming, and deleting files/directories
 * - Checking path existence and types
 * - Extracting path components (filename, extension, parent path)
 * - Normalizing paths and handling directory entries
 *
 * @author marciovmf
 * @version 1.0
 * @date 2024-10-29
 */


#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FS_PATH_MAX_LENGTH
# define FS_PATH_MAX_LENGTH 512
#endif  // FS_PATH_MAX_LENGTH

  typedef struct
  {
    char path[FS_PATH_MAX_LENGTH];
    size_t length;
  } FSPath;

  typedef struct
  {
    char name[FS_PATH_MAX_LENGTH]; 
    size_t size;
    time_t last_modified;
    int is_directory;
  } FSDirectoryEntry;


  /**
   * @brief Gets the current working directory.
   * 
   * @param path The FSPath structure to store the current working directory.
   * @return The length of the current working directory path.
   */
  size_t fs_cwd_get(FSPath* path);

  /**
   * @brief Sets the current working directory.
   * 
   * @param path The path to set as the current working directory.
   * @return The length of the new current working directory path.
   */
  size_t fs_cwd_set(const char* path);

  /**
   * @brief Sets the current working directory to the path of the executable.
   * 
   * @return The length of the new current working directory path.
   */
  size_t fs_cwd_set_from_executable_path(void);

  /**
   * @brief Copies a file to a new location.
   * 
   * @param file The path to the original file.
   * @param newFile The path where the file should be copied.
   * @return true if the copy operation succeeds, false otherwise.
   */
  bool fs_file_copy(const char* file, const char* newFile);

  /**
   * @brief Renames a file to a new name or location.
   * 
   * @param file The path to the original file.
   * @param newFile The new path or filename.
   * @return true if the rename operation succeeds, false otherwise.
   */
  bool fs_file_rename(const char* file, const char* newFile);

  /**
   * @brief Creates a directory at the specified path.
   * 
   * @param path The path where the directory should be created.
   * @return true if the directory creation succeeds, false otherwise.
   */
  bool fs_directory_create(const char* path);

  /**
   * @brief Recursively creates directories along a specified path.
   * 
   * @param path The path where the directories should be created, can be absolute or relative.
   * @return true if the directories are created successfully, false otherwise.
   */
  bool fs_directory_create_recursive(const char* path);

  /**
   * @brief Deletes a directory at the specified path.
   * 
   * @param directory The path of the directory to delete.
   * @return true if the deletion succeeds, false otherwise.
   */
  bool fs_directory_delete(const char* directory);

  /**
   * @brief Retrieves the path to the executable and stores it in an FSPath structure.
   * 
   * @param out The FSPath structure to store the executable's path.
   * @return The length of the executable path string.
   */
  size_t path_from_executable(FSPath* out);

  /**
   * @brief Checks if the given path points to a file.
   * 
   * @param path The path to check.
   * @return true if the path points to a file, false otherwise.
   */
  bool path_is_file(const char* path);

  /**
   * @brief Checks if the given path is a directory.
   * 
   * @param path The path to check.
   * @return true if the path is a directory, false otherwise.
   */
  bool path_is_directory(const char* path);

  /**
   * @brief Checks if the given path exists.
   * 
   * @param path The path to check.
   * @return true if the path exists, false otherwise.
   */
  bool path_exists(const char* path);

  /**
   * @brief Extracts the filename from a path.
   * 
   * @param path The path from which to extract the filename.
   * @return An Substr structure containing the filename.
   */
  Substr path_get_file_name(const char* path);

  /**
   * @brief Extracts the file extension from a path.
   * 
   * @param path The path from which to extract the file extension.
   * @return An Substr structure containing the file extension.
   */
  Substr path_get_file_extension(const char* path);

  /**
   * @brief Retrieves the parent path from a given path.
   * 
   * @param path The original path from which to extract the parent path.
   * @return An Substr containing the parent path. 
   *         The length will be set to 0 if the parent path does not exist.
   */
  Substr path_get_parent(const char* path);

  /**
   * @brief Checks if a path is absolute.
   * 
   * @param path The path to check.
   * @return true if the path is absolute, false if it is relative.
   */
  bool path_is_absolute(const char* path);

  /**
   * @brief Checks if a path is relative.
   * 
   * @param path The path to check.
   * @return true if the path is relative, false if it is absolute.
   */
  bool path_is_relative(const char* path);

  /**
   * @brief Normalizes a path by removing redundant parts, such as "." and "..".
   * 
   * @param path The path to normalize.
   */
  void path_normalize(FSPath* path);

  /**
   * @brief Sets the specified path into an FSPath structure.
   * 
   * @param out The FSPath structure to store the path.
   * @param path The path to set.
   * @return true if the path is valid, false otherwise.
   */
  bool path(FSPath* out, const char* path);

  /**
   * @brief Clones an FSPath structure into another.
   * 
   * @param out The destination FSPath structure.
   * @param path The source FSPath structure to clone.
   */
  void path_clone(FSPath* out, const FSPath* path);

  typedef struct FSDirectoryHandle_t FSDirectoryHandle;


  /**
   * @brief Opens a directory and finds the first file or directory within it.
   *
   * This function initializes a directory handle and retrieves the first entry in the specified directory.
   * The entry's details are populated in the provided DirectoryEntry structure.
   *
   * @param path The path of the directory to open. This can be an absolute or relative path.
   * @param entry A pointer to an DirectoryEntry structure that will be filled with information
   *    about the first file or directory found in the specified path.
   *
   * @return A pointer to an DirectoryHandle structure representing the opened directory, or NULL if the directory could not be opened.
   *    The caller is responsible for freeing the allocated memory.
   */
  FSDirectoryHandle* fs_find_first_file(const char* path, FSDirectoryEntry* entry);

  /**
   * @brief Finds the next file or directory in an open directory.
   *
   * This function retrieves the next entry in the directory represented by the specified DirectoryHandle.
   * The entry's details are populated in the provided DirectoryEntry structure.
   *
   * @param dirHandle A pointer to the DirectoryHandle structure that represents the opened directory.
   * @param entry A pointer to an DirectoryEntry structure that will be filled with information about the next file or directory found.
   *
   * @return 1 if a next entry was found and filled in the entry structure,
   *    or 0 if there are no more entries in the directory.
   */
  int fs_find_next_file(FSDirectoryHandle* dirHandle, FSDirectoryEntry* entry);

  /**
   * @brief Closes the directory handle and frees allocated resources.
   *
   * This function cleans up resources associated with the specified DirectoryHandle,
   * closing the directory stream and freeing any associated memory.
   *
   * @param dirHandle A pointer to the DirectoryHandle structure that represents 
   *                  the opened directory. This handle should have been obtained 
   *                  from a previous call to find_first_file().
   */
  void fs_find_close(FSDirectoryHandle* dirHandle);



/**
 * Gets the path to the system's temporary folder.
 *
 * @param path The FSPath structure to store the temporary folder path;
 * 
 * @return 0 on success, -1 on failure.
 */
int fs_get_temp_folder(FSPath* out);


#ifdef __cplusplus
}
#endif
#endif  // FILESYSTEM_H
