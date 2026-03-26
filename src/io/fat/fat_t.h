#ifndef FAT_T_H_
#define FAT_T_H_

#include <stdint.h>

/**
 * @brief FAT metadata structure
 *
 * Contains the metadata information for the FAT filesystem.
 */
typedef struct fat_metadata_st {
  /** Each block in the FAT has (2^{block_size}) * 256 bytes.
   * block_size should be in the range [0, 4].
   */
  uint8_t block_size;

  /** Total number of FAT blocks in the filesystem */
  uint8_t tot_blocks;
} fat_metadata_t;

/**
 * @brief FAT entry union
 *
 * A union that can represent a pointer to the next block in a FAT file chain or
 * the metadata for the FAT filesystem.
 */
typedef union fat_ut {
  /** For FAT[i] for i >= 1, this represents the pointer the the
   * next block in the file chain. */
  uint16_t next;

  /** For FAT[0], this represents metadata of the fat filesystem. */
  fat_metadata_t metadata;
} fat_t;

#endif  // FAT_T_H_
