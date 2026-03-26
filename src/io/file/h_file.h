#ifndef H_FILE_H_
#define H_FILE_H_

#include "io/fstd.h"

/**
 * @brief Opens a file with the specified mode on the host filesystem.
 *
 * Translates custom mode flags to Unix open flags:
 * - F_READ/F_EXEC: Opens read-only
 * - F_WRITE: Creates/truncates file for writing
 * - F_APPEND: Creates/appends to file
 *
 * @param fname Path to the file to open
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Host file descriptor on success, -1 on error
 */
int h_open(const char* fname, int mode);

/**
 * @brief Reads data from a host file descriptor
 *
 * @param fd Host file descriptor to read from
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error
 */
int h_read(int fd, void* buf, size_t nbytes);

/**
 * @brief Writes data to a host file descriptor
 *
 * Automatically flushes if writing to stdout, stderr
 * to ensure immediate output visibility.
 *
 * @param fd Host file descriptor to write to
 * @param buf Buffer containing data to write
 * @param nbytes Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
int h_write(int fd, const void* buf, size_t nbytes);

/**
 * @brief Closes a host file descriptor
 *
 * @param fd Host file descriptor to close
 * @return 0 on success, -1 on error
 */
int h_close(int fd);

/**
 * @brief Sets the file offset for the given host file descriptor
 *
 * @param fd Host file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 */
void h_lseek(int fd, int offset, int whence);

#endif  // H_FILE_H_