/**
 * @file k_path.h
 * @brief Kernel-level directory and path operations for PennFAT.
 *
 * This header provides the kernel-level interface for directory operations
 * in the PennFAT filesystem. It supports searching, adding, updating, and
 * removing directory entries, as well as listing directory contents.
 *
 * @note PennFAT uses a flat directory structure with a single root directory.
 *       All files reside directly under the root.
 */

#ifndef K_PATH_H_
#define K_PATH_H_

#include "io/fat/file_t.h"

/**
 * @brief Get a pointer to the root directory.
 *
 * Returns a reference to the global root directory structure.
 * The root directory is the only directory in PennFAT.
 *
 * @return Pointer to the root directory file_t structure.
 */
file_t* k_get_root_dir();

/**
 * @brief Search for a file in a directory.
 *
 * Searches the specified directory for a file with the given name.
 * If found, optionally copies the file entry to the output parameter.
 *
 * @param fname The filename to search for.
 * @param dir The directory to search in.
 * @param[out] file Output: the file entry if found (can be NULL).
 * @return The index of the file entry if found, -1 otherwise.
 */
int k_dir_search(const char* fname, file_t* dir, file_t* file);

/**
 * @brief Update a directory entry at a given index.
 *
 * Overwrites the directory entry at the specified index with
 * the provided file structure.
 *
 * @param dir The directory to update.
 * @param idx The index of the entry to update.
 * @param file The new file entry data.
 */
void k_dir_upd_entry(file_t* dir, int idx, file_t file);

/**
 * @brief Remove a directory entry at a given index.
 *
 * Removes the entry at the specified index and shifts all subsequent
 * entries up to fill the gap. Writes a null terminator after the last entry.
 *
 * @param dir The directory to modify.
 * @param idx The index of the entry to remove.
 */
void k_dir_rem_entry(file_t* dir, int idx);

/**
 * @brief Add a new entry to a directory.
 *
 * Appends a new file entry to the directory. Searches for the first
 * empty slot (null-terminated name) or appends at the end.
 *
 * @param dir The directory to add the entry to.
 * @param file The file entry to add.
 * @return 0 on success, -1 on error.
 */
int k_dir_add_entry(file_t* dir, file_t file);

/**
 * @brief List directory contents.
 *
 * Lists files in the root directory. If a filename is provided, lists
 * only that file. Otherwise, lists all files. This is an alias for s_ls().
 *
 * @param filename The specific file to list, or NULL for all files.
 * @return 0 on success, -1 on error.
 */
int k_ls(const char* filename);

#endif  // K_PATH_H_