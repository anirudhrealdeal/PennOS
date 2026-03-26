/**
 * @file s_fat.h
 * @brief System-level FAT filesystem management interface
 *
 * Provides user/system-level wrappers for FAT filesystem operations.
 * These functions serve as the interface between user applications and
 * the kernel-level FAT implementation.
 */

#ifndef S_FAT_H_
#define S_FAT_H_

#include "io/fstd.h"

/**
 * @copydoc k_mkfs
 */
void s_mkfs(const char* fs_name,
            size_t blocks_in_fat,
            size_t block_size_config);

/**
 * @copydoc k_mount
 */
int s_mount(const char* filename);

/**
 * @copydoc k_unmount
 */
void s_unmount();

/**
 * @copydoc k_has_mount
 */
bool s_has_mount();

#endif  // S_FAT_H_