/**
 * @file h_file.c
 * @brief Implementation of host file I/O wrapper functions
 *
 * Wraps standard Unix file operations with custom mode flags
 * defined in fstd.h (F_READ, F_WRITE, F_APPEND, F_EXEC, etc.).
 */

#include "h_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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
 * @return File descriptor on success, -1 on error
 */
int h_open(const char* fname, int mode) {
  int flags;

  if (mode == F_READ || mode == F_EXEC) {
    /* Open for read only */
    flags = O_RDONLY;
  } else if (mode == F_WRITE) {
    /* Open for write create if doesnt exist and truncate if doesnt exist */
    flags = O_WRONLY | O_CREAT | O_TRUNC;
  } else if (mode == F_APPEND) {
    /* Open for append and create if doesnt exist */
    flags = O_WRONLY | O_CREAT | O_APPEND;
  } else {
    /* Invalid mode */
    return -1;
  }

  /* Same permissions as mkfs */
  return open(fname, flags, 0644);
}

/**
 * @brief Reads data from a host file descriptor
 *
 * @param fd Host file descriptor to read from
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error
 * @see the standard read() function for additional details
 */
int h_read(int fd, void* buf, size_t nbytes) {
  return read(fd, buf, nbytes);
}

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
int h_write(int fd, const void* buf, size_t nbytes) {
  int res = write(fd, buf, nbytes);
  if ((fd == STDOUT_FILENO || fd == STDERR_FILENO) && res > 0) {
    fflush((fd == STDOUT_FILENO) ? stdout : stderr);
  }
  return res;
}

/**
 * @brief Closes a host file descriptor
 *
 * @param fd File descriptor to close
 * @return 0 on success, -1 on error
 */
int h_close(int fd) {
  return close(fd);
}

/**
 * @brief Sets the file offset for the given host file descriptor
 *
 * @param fd Host file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 */
void h_lseek(int fd, int offset, int whence) {
  /* Translate to host whence */
  whence = (whence == F_SEEK_SET)   ? SEEK_SET
           : (whence == F_SEEK_CUR) ? SEEK_CUR
           : (whence == F_SEEK_END) ? SEEK_END
                                    : -1;
  if (whence == -1) {
    return;
  }
  lseek(fd, offset, whence);
}