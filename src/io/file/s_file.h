/**
 * @file s_file.h
 * @brief System call interface for file I/O operations
 *
 * Provides user-level system call wrappers for file operations that interface
 * with the kernel file layer. These functions perform process-specific file
 * descriptor management and delegate to kernel functions (k_file.h) for actual
 * filesystem operations.
 */

#ifndef S_FILE_H_
#define S_FILE_H_

#include "io/fstd.h"

/**
 * @brief Opens a file in the fat filesystem
 *
 * @param fname Path to the file in the filesystem
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Process-local file descriptor on success, -1 on error
 * @see k_open for documentation about mode behavior
 */
int s_open(const char* fname, int mode);

/**
 * @brief Opens an external host file
 *
 * @param fname Path to the external file
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Process-local file descriptor on success, -1 on error
 * @see h_open for documentation about mode behavior
 */
int s_ext_open(const char* fname, int mode);

/**
 * @brief Reads data from a file
 *
 * @param fd Process-local file descriptor
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error, 0 on EOF
 */
int s_read(int fd, void* buf, size_t nbytes);

/**
 * @brief Writes data to a file
 *
 * @param fd Process-local file descriptor
 * @param buf Buffer containing data to write
 * @param nbytes Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
int s_write(int fd, const void* buf, size_t nbytes);

/**
 * @brief Closes a file descriptor (system call interface)
 *
 * If called from a process context, releases the process-local file
 * descriptor which decrements the reference count on the kernel file
 * descriptor. If called without a process context (pid == -1), directly
 * closes the kernel file descriptor.
 *
 * @param fd Process-local file descriptor to close
 * @return 0 on success, -1 on error
 */
int s_close(int fd);

/**
 * @brief Deletes a file from the filesystem
 *
 * @param fname Path to the file to delete
 * @return 0 on success, -1 on error
 * @see k_unlink for more details
 */
int s_unlink(const char* fname);

/**
 * @brief Sets the file offset
 *
 * @param fd Process-local file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 * @see k_lseek for more details
 */
void s_lseek(int fd, int offset, int whence);

/**
 * @brief Renames a file in the filesystem
 *
 * @param source Current file name
 * @param dest New file name
 * @return 0 on success, -1 on error
 * @see k_mv for more details
 */
int s_mv(const char* source, const char* dest);

/**
 * @brief Changes the file permissions of the provided file
 *
 * Changes the file permissions of the provided file (1=execute, 2=write,
 * 4=read).
 *
 * @param filename Path to the file
 * @param flag Permission bit to modify (1=execute, 2=write, 4=read)
 * @param add 1 to add permission, 0 to remove permission
 * @return 0 on success, -1 on error (file not found)
 */
int s_chmod(const char* filename, unsigned char flag, int add);

/**
 * @brief Gets the open mode of a file descriptor
 *s
 * @param fd Process-local file descriptor
 * @return Open mode on success, -1 on error
 */
int s_get_open_mode(int fd);

/**
 * @brief Checks if two file descriptors refer to the same file
 *
 * @param fd1 First process-local file descriptor
 * @param fd2 Second process-local file descriptor
 * @return true if both refer to the same file, false otherwise
 */
bool s_same_source(int fd1, int fd2);

#endif  // S_FILE_H_