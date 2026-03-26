/**
 * @file k_fat.c
 * @brief Kernel-level FAT filesystem management implementation
 *
 * Implements a File Allocation Table (FAT) filesystem with block allocation,
 * mounting/unmounting, and filesystem creation capabilities. The FAT is
 * memory-mapped for efficient access.
 */

#include "k_fat.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "io/path/k_path.h"
#include "io/term/s_term.h"
#include "process/schedule/k_schedule.h"

/** @brief FAT entry marking end of file/chain */
static const fat_t FAT_EOF = {0xFFFF};

/** @brief FAT entry marking empty/unallocated block */
static const fat_t FAT_EMPTY = {0};

/** @brief Flag indicating if a filesystem is currently mounted */
static bool fs_mounted = false;

/** @brief File descriptor of the mounted filesystem, or -1 if unmounted */
static int fs_fd = -1;

/** @brief Pointer to memory-mapped FAT region */
static fat_t* fat = NULL;

/**
 * @brief Get the byte size of FAT blocks based on configuration
 *
 * Converts a block size configuration index to the actual byte size.
 * Supported sizes: 256, 512, 1024, 2048, 4096 bytes.
 *
 * @param block_size_config Configuration index (0-4)
 * @return Block size in bytes, or -1 if configuration is invalid
 */
size_t k_get_block_size(int block_size_config) {
  static size_t sizes[5] = {256, 512, 1024, 2048, 4096};
  if (block_size_config < 0 || block_size_config >= 5) {
    s_perror("k_get_block_size: invalid block size config\n");
    return -1;
  }
  return sizes[block_size_config];
}

/**
 * @brief Given a valid FAT header, calculates the total size of the FAT region
 * in bytes.
 *
 * @param fat_blk A valid FAT header containing metadata.
 * @return Total FAT size in bytes
 */
size_t k_sizeof_fat(fat_t fat_blk) {
  return k_get_block_size(fat_blk.metadata.block_size) *
         (size_t)fat_blk.metadata.tot_blocks;
}

/**
 * @brief Release an entire chain of FAT blocks
 *
 * Traverses the FAT chain starting from the given block, releasing
 * each block back to the free pool. Stops when reaching EOF (0xFFFF).
 *
 * @param start Starting block number of the chain
 */
void k_release_fat_chain(uint16_t start) {
  uint16_t curr = start;
  while (curr != 0xFFFF) {
    uint16_t nex = k_get_fat(curr)->next;
    k_release_block(curr);
    curr = nex;
  }
}

/**
 * @brief Get the block size of the currently mounted filesystem
 *
 * @return Block size in bytes of the current filesystem, or -1 if unmounted.
 */
size_t k_get_blk_size() {
  if (!fs_mounted) {
    return -1;
  }
  return k_get_block_size(fat->metadata.block_size);
}

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
int k_get_data_pos(uint16_t blk, int offset) {
  return k_sizeof_fat(*fat) + k_get_blk_size() * (blk - 1) + offset;
}

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
fat_t* k_get_fat(uint16_t blk) {
  ASSERT_NO_INTERRUPT("k_get_fat");
  return fat + blk;
}

/**
 * @brief Check if a filesystem is currently mounted
 *
 * @return true if mounted, false otherwise
 */
bool k_has_mount() {
  return fs_mounted;
}

/**
 * @brief Get the file descriptor of the mounted filesystem
 *
 * @return File descriptor, or -1 if no filesystem is mounted
 */
int k_get_fat_fd() {
  return fs_fd;
}

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
            size_t block_size_config) {
  /* Error according to specification requirements */
  if (fs_mounted) {
    s_perror("unexpected command");
    return;
  }

  /* Input validation */
  if (blocks_in_fat < 1 || blocks_in_fat > 32) {
    s_perror("k_mkfs: blocks_in_fat must be beween 1 and 32\n");
    return;
  }
  if (block_size_config < 0 || block_size_config > 4) {
    s_perror("k_mkfs: block_size_config must be between 0 and 4\n");
    return;
  }

  /* Prepare file for writing */
  int fd = open(fs_name, O_CREAT | O_TRUNC | O_RDWR, 0644);
  if (fd < 0) {
    s_perror("k_mkfs: open error\n");
  }
  if (lseek(fd, 0, SEEK_SET) < 0) {
    s_perror("k_mkfs: lseek failed\n");
    close(fd);
  }

  /* Metadata relevant for creating the FAT filesystem */
  size_t block_size = k_get_block_size(block_size_config);
  size_t fat_size = block_size * blocks_in_fat;
  size_t fat_entries = block_size * blocks_in_fat / sizeof(fat_t);
  size_t data_region_size = block_size * (fat_entries - 1);

  /* Write out the fat header */
  fat_t header = (fat_t){.metadata = {
                             .tot_blocks = blocks_in_fat,
                             .block_size = block_size_config,
                         }};
  if (write(fd, &header, sizeof(fat_t)) < 0) {
    s_perror("k_mkfs: write failed header\n");
    close(fd);
  }

  /* Add the FAT entry for the root directory */
  if (write(fd, &FAT_EOF, sizeof(fat_t)) < 0) {
    s_perror("k_mkfs: write failed root directory\n");
    close(fd);
  }

  /* Ensure all other FAT entries are marked as empty */
  for (size_t i = 2; i < fat_entries; i++) {
    if (write(fd, &FAT_EMPTY, sizeof(fat_t)) < 0) {
      s_perror("k_mkfs: write rest of data region error\n");
      close(fd);
    }
  }

  /* Grow the file to include the (currently empty) data region */
  if (lseek(fd, fat_size + data_region_size - 1, SEEK_SET) < 0) {
    s_perror("k_mkfs: lseek failed\n");
  }
  char c = '\0';
  write(fd, &c, 1);

  if (close(fd) < 0) {
    s_perror("k_mkfs: final close error\n");
  }
}

/**
 * @brief Mount an existing FAT filesystem
 *
 * Opens and prepares the specified filesystem file, making it available
 * for file operations. Only one filesystem can be mounted at a time.
 *
 * @param fs_name Path to the filesystem file to mount
 * @return 0 on success, -1 on error (e.g., file not found, already mounted)
 */
int k_mount(const char* fs_name) {
  if (fs_mounted) {
    s_perror("unexpected command");
    return -1;
  }

  /* The global variable fs_fd represents the global fd of the filesystem */
  fs_fd = open(fs_name, O_RDWR);
  if (fs_fd < 0) {
    s_perror("mount: could not open file %s\n", fs_name);
    return -1;
  }

  /* Read out the header */
  fat_t header;
  read(fs_fd, &header, sizeof(fat_t));

  /* Create a MMAP of the FAT region, not including the data region */
  size_t sz = k_sizeof_fat(header);
  fat = (fat_t*)mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);

  fs_mounted = true;

  /* Traverses FAT to figure out the size of the root directory, rounded up to
   * the nearest full block. */
  file_t* root_dir = k_get_root_dir();
  root_dir->size = 0;
  uint16_t curr = root_dir->first_block.next;
  while (curr != 0xFFFF) {
    root_dir->size++;
    curr = k_get_fat(curr)->next;
  }
  root_dir->size *= k_get_blk_size();

  return 0;
}

/**
 * @brief Unmount the currently mounted filesystem
 *
 * Unmaps the FAT from memory and closes the filesystem file descriptor. All
 * changes are synchronized to disk before unmounting.
 *
 * @return 0 on success, -1 on error (e.g., no filesystem mounted)
 * @warning Does not check if files are currently open/in use
 */
int k_unmount() {
  if (!fs_mounted) {
    s_perror("unexpected command");
    return -1;
  }

  munmap(fat, k_sizeof_fat(*fat));
  close(fs_fd);
  fat = NULL;
  fs_fd = -1;
  fs_mounted = false;
  return 0;
}

/**
 * @brief Allocate a new block from the filesystem
 *
 * Allocates the lowest-indexed free FAT block from the filesystem.
 *
 * @pre Interrupts are disabled
 * @return Block number of the allocated block, or 0xFFFF if no free blocks
 */
uint16_t k_request_block() {
  ASSERT_NO_INTERRUPT("k_request_block");
  size_t num_entries =
      (size_t)fat->metadata.tot_blocks * k_get_blk_size() / sizeof(fat_t);

  /* Finds the first block marked as empty. Indices 0 and 1 are reserved for
   * metadata and root. */
  for (size_t i = 2; i < num_entries; i++) {
    if (fat[i].next == FAT_EMPTY.next) {
      fat[i] = FAT_EOF;
      return (uint16_t)i;
    }
  }
  return 0xFFFF;
}

/**
 * @brief Release a FAT block back to the free pool
 *
 * Marks the specified block as empty (available for reallocation).
 *
 * @pre Interrupts are disabled
 * @param blk Block number to release. Must be >= 2.
 */
void k_release_block(uint16_t blk) {
  ASSERT_NO_INTERRUPT("k_release_block");
  if (blk <= 1) {
    s_perror("k_release_block: Attempted released of protected FAT entry\n");
    return;
  }
  fat[blk] = FAT_EMPTY;
}

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
void k_start_block(file_t* file) {
  ASSERT_NO_INTERRUPT("k_start_block");
  if (file->type == 2) {
    s_perror("k_start_block: not implemented\n");
  } else {
    /* Refresh file entry in memory from filesystem */
    int idx = k_dir_search(file->name, k_get_root_dir(), file);
    if (idx == -1) {
      s_perror("k_start_block: unexpected missing file\n");
    }

    /* Attempt to allocate a block for the file */
    if (file->first_block.next == 0xFFFF) {
      file->first_block.next = k_request_block();
      k_dir_upd_entry(k_get_root_dir(), idx, *file);
    }
  }
}
