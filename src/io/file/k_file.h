#ifndef K_FILE_H_
#define K_FILE_H_

#include "io/fstd.h"

/**
 * @brief Opens a file in the FAT filesystem in the provided mode
 *
 * Opens a file using the provided mode.
 *
 * The modes work as follows:
 *  - F_READ / F_EXEC = Readonly access to file, errors if doesn't exist.
 *  - F_WRITE = Write access to file, truncates the file if exists.
 *  - F_APPEND = Write access to file, subsequent writes extend the file.
 *
 * Will error if the file exists and has the wrong permission. Only one open can
 * have write capability to a file at a time, subsequent opens will error.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the file in the filesystem
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Kernel file descriptor on success, -1 on error
 * @note Emits perror messages upon error with the relevant information
 */
int k_open(const char* fname, int mode);

/**
 * @brief Opens an external host file.
 *
 * Opens a file from the host filesystem and registers it in the
 * kernel file descriptor table for unified access.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the external file
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Kernel file descriptor on success, -1 on error
 * @see h_open for how modes are handled
 */
int k_ext_open(const char* fname, int mode);

/**
 * @brief Reads data from a kernel file descriptor for a local fs file.
 *
 * @pre fd references a local file
 * @pre Interrupts are disabled
 * @param fd File descriptor to read from
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error
 * @see standard read() function for details
 * @see k_ext_read for reading from host file descriptors
 */
int k_read(int fd, void* buf, size_t nbytes);

/**
 * @copydoc h_read
 */
int k_ext_read(int fd, void* buf, size_t nbytes);

/**
 * @brief Writes data to a kernel file descriptor
 *
 * @pre Interrupts are disabled
 * @param fd Kernel file descriptor
 * @param buf Buffer containing data to write
 * @param nbytes Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
int k_write(int fd, const void* buf, size_t nbytes);

/**
 * @copydoc k_fd_close
 */
int k_close(int fd);

/**
 * @brief Deletes a file from the filesystem
 *
 * Deletes a file from the filesystem. Fails if the file is currently open by
 * any process.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the file to delete
 * @return 0 on success, -1 on error (file open or not found)
 */
int k_unlink(const char* fname);

/**
 * @brief Sets the file offset for the given file descriptor
 *
 * Sets the file offset for the given file descriptor. The whence operator acts
 * the same as whence for standard lseek on the subset of options F_SEEK_SET,
 * F_SEEK_CUR, and F_SEEK_END.
 *
 * @pre Interrupts are disabled
 * @param fd Kernel file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 */
void k_lseek(int fd, int offset, int whence);

/**
 * @brief Renames a file in the filesystem
 *
 * Renames a file to the new name, replacing the destination file if it exists.
 * Will error if the source file doesn't exist, if either file is open, or if
 * the source and destination are the same.
 *
 * @pre Interrupts are disabled
 * @param source Current file name
 * @param dest New file name
 * @return 0 on success, -1 on error (file open, not found, or cannot add)
 */
int k_mv(const char* source, const char* dest);

#endif  // K_FILE_H_