/**
 * @file u_file.h
 * @brief User-level file manipulation commands for the PennOS shell.
 *
 * This header declares the built-in shell commands for file operations.
 * Each function follows the spthread entry point signature (void* -> void*)
 * and expects argv-style arguments passed via the arg parameter.
 */

#ifndef U_FILE_H_
#define U_FILE_H_

/**
 * @brief Concatenate and display file contents.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_cat(void* arg);

/**
 * @brief Create files or update timestamps.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_touch(void* arg);

/**
 * @brief Move or rename a file.
 *
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_mv(void* arg);

/**
 * @brief Copy a file with optional host filesystem support.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_cp(void* arg);

/**
 * @brief Remove files from the filesystem.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_rm(void* arg);

/**
 * @brief List directory contents.
 *
 * Calls s_ls to display file information.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_ls(void* arg);

/**
 * @brief Change file permissions.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_chmod(void* arg);

#endif  // U_FILE_H_