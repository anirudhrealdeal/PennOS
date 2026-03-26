/**
 * @file k_filetable.h
 * @brief Kernel file descriptor table management API.
 *
 * This header provides the kernel-level interface for managing file descriptors
 * in PennOS. It supports both PennFAT filesystem files and host OS files,
 * allowing the kernel to handle I/O operations, file seeking, and descriptor
 * duplication/sharing between processes.
 *
 * @note Functions prefixed with `k_fd_` operate on the global kernel file
 *       descriptor table. Functions prefixed with `k_lfd_` operate on
 *       process-local file descriptor mappings.
 */

#ifndef K_FILE_TABLE_H_
#define K_FILE_TABLE_H_

#include "io/fat/file_t.h"

/**
 * @brief Initialize the kernel file descriptor table.
 *
 * @return 0 on success.
 */
int k_fd_init();

/**
 * @brief Free the kernel file descriptor table.
 */
void k_fd_clean();

/**
 * @brief Allocate a kernel file descriptor for a file.
 *
 * Creates a new file descriptor entry for the given file with the specified
 * access mode.
 *
 * @pre Interrupts are disabled
 * @param file The file to open.
 * @param mode Access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC).
 * @return The allocated kernel file descriptor index.
 */
int k_fd_alloc(file_t file, int mode);

/**
 * @brief Allocate a kernel file descriptor for a host fd.
 *
 * Creates a new file descriptor entry that wraps a host fd.
 *
 * @pre Interrupts are disabled
 * @param hfd The host OS file descriptor.
 * @param mode Access mode (F_READ, F_WRITE, etc.).
 * @return The allocated kernel file descriptor index.
 */
int k_fd_alloc_h(int hfd, int mode);

/**
 * @brief Check if a file descriptor entry exists and is valid.
 *
 * @param fd The kernel file descriptor.
 * @return true if the descriptor is valid and has a positive refcount.
 */
bool k_fd_has_entry(int fd);

/**
 * @brief Get the host file descriptor if this is a host file.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The host file descriptor if it is a host file, else -1
 */
int k_fd_get_hfd(int fd);

/**
 * @brief Duplicate a kernel file descriptor.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor to duplicate.
 * @return The same file descriptor on success.
 */
int k_fd_dup(int fd);

/**
 * @brief Close a kernel file descriptor.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor to close.
 * @return 0 on success.
 */
int k_fd_close(int fd);

/**
 * @brief Apply pending offset changes to a file descriptor.
 *
 * @param fd The file descriptor to update.
 * @return 0 on success, -1 on error (EOF in read mode or allocation failure).
 */
int apply_offset(int fd);

/**
 * @brief Walk through file blocks for I/O operations.
 *
 * Advances through the file's FAT chain, returning the position and size
 * of the next chunk to read/write.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @param[in,out] target_size Remaining bytes to process; updated after call.
 * @param[out] pos Output: position in the data region for this chunk.
 * @param[out] size Output: number of bytes available in this chunk.
 * @return 0 on success, -1 on error.
 */
int k_fd_walk(int fd, size_t* target_size, int* pos, size_t* size);

/**
 * @brief Get the current file offset.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The current target offset in bytes.
 */
uint32_t k_fd_get_offset(int fd);

/**
 * @brief Set the file offset.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @param off The new offset in bytes.
 */
void k_fd_set_offset(int fd, int off);

/**
 * @brief Get the size of the file.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The file size in bytes.
 */
int k_fd_get_size(int fd);

/**
 * @brief Get the access mode of a file descriptor.
 *
 * @param fd The kernel file descriptor.
 * @return The access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC).
 */
int k_fd_mode(int fd);

/**
 * @brief Get a reference to the file referenced by a fd.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return Pointer to the file_t structure.
 */
file_t* k_fd_file_ref(int fd);
/**
 * @brief Add a kernel file descriptor to a process's local table.
 *
 * Maps a global kernel fd to a process-local fd number.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param hfd The kernel file descriptor to add.
 * @return The local file descriptor number assigned.
 */
int k_lfd_put(int pid, int hfd);

/**
 * @brief Release a local file descriptor from a process.
 *
 * Removes the mapping and closes the underlying kernel fd.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param fd The local file descriptor to release.
 * @return 0 on success, -1 if invalid.
 */
int k_lfd_release(int pid, int fd);

/**
 * @brief Get the kernel file descriptor from a local descriptor.
 *
 * Translates a process-local fd to the global kernel fd.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param fd The local file descriptor.
 * @return The kernel file descriptor, or -1 if invalid.
 */
int k_lfd_get(int pid, int fd);

/**
 * @brief Check if a file is currently open.
 *
 * @pre Interrupts are disabled
 * @param fname The filename to check.
 * @return 1 if the file is open, 0 otherwise.
 */
int k_fd_is_open(const char* fname);

/**
 * @brief Check if a file is open with write or append mode.
 *
 * Used to enforce write locking - only one writer allowed at a time.
 *
 * @pre Interrupts are disabled
 * @param fname The filename to check.
 * @return 1 if the file is write-locked, 0 otherwise.
 */
int k_fd_is_write_locked(const char* fname);

#endif  // K_FILE_TABLE_H_