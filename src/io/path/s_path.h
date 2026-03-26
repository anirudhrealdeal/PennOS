/**
 * @file s_path.h
 * @brief System-level path and directory listing interface.
 *
 * This header provides the system call interface for directory listing
 * in PennOS. It is used by user programs and the shell to display
 * directory contents.
 */

#ifndef S_PATH_H_
#define S_PATH_H_

/**
 * @brief List files in the root directory.
 *
 * If a filename is provided, displays information for that specific file.
 * Otherwise, lists all files in the root directory with their permissions,
 * size, modification time, and name.
 *
 * @param filename The specific file to list, or NULL to list all files.
 * @return 0 on success, -1 on error.
 */
int s_ls(const char* filename);

#endif  // S_PATH_H_