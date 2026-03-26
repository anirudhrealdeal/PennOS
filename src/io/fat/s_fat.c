/**
 * @file s_fat.c
 * @brief System-level FAT filesystem management implementation
 *
 * Implements system-level wrappers for FAT filesystem operations. These
 * functions provide a clean interface for user-space applications while
 * delegating actual work to the kernel-level implementations.
 */

#include "s_fat.h"
#include "k_fat.h"

/**
 * @copydoc k_mkfs
 */
void s_mkfs(const char* fs_name,
            size_t blocks_in_fat,
            size_t block_size_config) {
  return k_mkfs(fs_name, blocks_in_fat, block_size_config);
}

/**
 * @copydoc k_mount
 */
int s_mount(const char* filename) {
  return k_mount(filename);
}

/**
 * @copydoc k_unmount
 */
void s_unmount() {
  k_unmount();
}

/**
 * @copydoc k_has_mount
 */
bool s_has_mount() {
  return k_has_mount();
}