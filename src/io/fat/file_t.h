#ifndef FILE_T_H_
#define FILE_T_H_

#include <time.h>
#include "fat_t.h"

/**
 * @brief File Structure for the FAT filesystem
 *
 * Represents a file entry in the FAT filesystem, containing file metadata and a
 * pointer to the first block of the given file.
 */
typedef struct file_st {
  char name[32];       /**< File name */
  uint32_t size;       /**< File size in bytes */
  fat_t first_block;   /**< First block of the file */
  uint8_t type;        /**< The type of file according to specification */
  uint8_t perm;        /**< Standard permissions of the file */
  time_t mtime;        /**< Last modification time */
  char __reserved[16]; /**< Reserved space for future use */
} file_t;

#endif  // FILE_T_H_