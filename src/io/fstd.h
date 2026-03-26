/**
 * @file fstd.h
 * @brief File system standard definitions and constants.
 *
 * This header defines constants used throughout PennOS for file operations,
 * including file access modes, seek positions, and standard file descriptors.
 */

#ifndef _FSTD_H
#define _FSTD_H

#include <stddef.h>

/**
 * @name File Access Modes
 * @brief Constants for specifying how a file is opened.
 * @{
 */
#define F_WRITE 0
#define F_READ 1
#define F_APPEND 2
#define F_EXEC 3
/** @} */
/**
 * @name Seek Whence Constants
 * @brief Constants for lseek() position reference.
 * @{
 */
#define F_SEEK_SET 0
#define F_SEEK_CUR 1
#define F_SEEK_END 2
/** @} */

/**
 * @name Standard File Descriptors
 * @brief Process-local file descriptor numbers for standard I/O streams.
 * @{
 */
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif
/** @} */


#endif  // _FSTD_H