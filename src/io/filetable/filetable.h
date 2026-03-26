/**
 * @file filetable.h
 * @brief Data structures for the kernel file descriptor table.
 *
 * This header defines the core data structures used by the kernel to manage
 * open files and file descriptors in the PennOS operating system. It provides
 * two key structures: one for tracking globally open files and another for
 * individual file descriptor entries.
 */

#ifndef FILE_TABLE_H_
#define FILE_TABLE_H_

#include "io/fat/file_t.h"
#include "io/fstd.h"

/**
 * @brief Entry in the global open file table.
 *
 * Represents a file that is currently open in the system. Multiple file
 * descriptors may reference the same open file entry.
 */
typedef struct {
  file_t file;    /**< The underlying FAT file structure. */
  int refcount;   /**< Number of file descriptors referencing this file. */
} k_open_file_entry_t;

/**
 * @brief Entry in the kernel file descriptor table.
 *
 * Represents a single file descriptor in the kernel, tracking the open file
 * it refers to, access mode, current position, and reference count for
 * descriptor sharing (e.g., after dup or fork).
 */
typedef struct {
  int open_file_idx;    /**< Index into k_active_files (-1 for host files). */
  int hfd;              /**< Host file descriptor (-1 for PennFAT files). */
  int refcount;         /**< Number of references to this descriptor. */
  int mode;             /**< Access mode (F_READ, F_WRITE, F_APPEND, F_EXEC). */
  int target_offset;    /**< Requested file offset for next I/O operation. */
  int virtual_offset;   /**< Actual current offset in file. */
  uint16_t curr_block;  /**< Current FAT block for file traversal. */
} k_fd_entry_t;

#endif  // FILE_TABLE_H_