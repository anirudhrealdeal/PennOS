/**
 * @file k_fat.h
 * @brief Kernel-level FAT filesystem management interface
 *
 * Provides functions for creating, mounting, and managing a FAT (File
 * Allocation Table) filesystem. This includes block allocation, FAT chain
 * management, and filesystem lifecycle operations.
 */

#ifndef K_FAT_H_
#define K_FAT_H_

#include <stddef.h>
#include "fat_t.h"
#include "file_t.h"

/**
 * @brief Get the byte size of FAT blocks based on configuration
 *
 * Converts a block size configuration index to the actual byte size.
 * Supported sizes: 256, 512, 1024, 2048, 4096 bytes.
 *
 * @param block_size_config Configuration index (0-4)
 * @return Block size in bytes, or -1 if configuration is invalid
 */
size_t k_get_block_size(int block_size_config);

/**
 * @brief Given a valid FAT header, calculates the total size of the FAT region
 * in bytes.
 *
 * @param fat_blk A valid FAT header containing metadata.
 * @return Total FAT size in bytes
 */
size_t k_sizeof_fat(fat_t fat_blk);

/**
 * @brief Release an entire chain of FAT blocks
 *
 * Traverses the FAT chain starting from the given block, releasing
 * each block back to the free pool. Stops when reaching EOF (0xFFFF).
 *
 * @param start Starting block number of the chain
 */
void k_release_fat_chain(uint16_t start);

/**
 * @brief Get the block size of the currently mounted filesystem
 *
 * @return Block size in bytes of the current filesystem, or -1 if unmounted.
 */
size_t k_get_blk_size();

/**
 * @brief Get the actual memory offset of file data in the filesystem.
 *
 * Given a FAT block index and memory offset within that block, computes the
 * byte offset relative to the start of the filesystem file where this memory is
 * located.
 *
 * @param blk FAT block number
 * @param offset Offset within the block
 * @return Absolute file position in bytes
 */
int k_get_data_pos(uint16_t blk, int offset);

/**
 * @brief Get a pointer to a FAT entry
 *
 * Returns a pointer to the FAT entry for the specified block number. All reads
 * and writes to a fat block should be done in the same atomic block as
 * obtaining the pointer.
 *
 * @pre Interrupts are disabled
 * @param blk Index of FAT block entry to reference
 * @return Pointer to FAT entry
 */
fat_t* k_get_fat(uint16_t blk);

/**
 * @brief Check if a filesystem is currently mounted
 *
 * @return true if mounted, false otherwise
 */
bool k_has_mount();

/**
 * @brief Get the file descriptor of the mounted filesystem
 *
 * @return File descriptor, or -1 if no filesystem is mounted
 */
int k_get_fat_fd();

/**
 * @brief Create and initialize a new FAT filesystem
 *
 * Creates a new filesystem file with the specified parameters. The file
 * is initialized with:
 * - FAT header containing metadata
 * - Root directory entry (block 1) marked as EOF
 * - All other blocks marked as empty
 * - Data region sized according to the number of FAT entries
 *
 * @param fs_name Path where the filesystem file will be created
 * @param blocks_in_fat Number of blocks in FAT [1-32]
 * @param block_size_config Block size configuration index [0-4]
 */
void k_mkfs(const char* fs_name,
            size_t blocks_in_fat,
            size_t block_size_config);

/**
 * @brief Mount an existing FAT filesystem
 *
 * Opens and prepares the specified filesystem file, making it available
 * for file operations. Only one filesystem can be mounted at a time.
 *
 * @param fs_name Path to the filesystem file to mount
 * @return 0 on success, -1 on error (e.g., file not found, already mounted)
 */
int k_mount(const char* fs_name);

/**
 * @brief Unmount the currently mounted filesystem
 *
 * Unmaps the FAT from memory and closes the filesystem file descriptor. All
 * changes are synchronized to disk before unmounting.
 *
 * @return 0 on success, -1 on error (e.g., no filesystem mounted)
 * @warning Does not check if files are currently open/in use
 */
int k_unmount();

/**
 * @brief Allocate a new block from the filesystem
 *
 * Allocates the lowest-indexed free FAT block from the filesystem.
 *
 * @pre Interrupts are disabled
 * @return Block number of the allocated block, or 0xFFFF if no free blocks
 */
uint16_t k_request_block();

/**
 * @brief Release a FAT block back to the free pool
 *
 * Marks the specified block as empty (available for reallocation).
 *
 * @pre Interrupts are disabled
 * @param blk Block number to release. Must be >= 2.
 */
void k_release_block(uint16_t blk);

/**
 * @brief Ensure a file has at least one block allocated
 *
 * If the file's first block is 0xFFFF (unallocated), requests a new block
 * from the filesystem and updates the directory entry. This is typically
 * called before writing to a newly created or empty file.
 *
 * @pre Interrupts are disabled
 * @param file Pointer to the file structure to check/allocate
 */
void k_start_block(file_t* file);

#endif  // K_FAT_H