/**
 * @file k_filetable.c
 * @brief Implementation of the kernel file descriptor table.
 *
 * This file implements the kernel-level file descriptor management for PennOS.
 * It maintains two core data structures:
 * - **k_active_files**: A table of open files shared across all descriptors.
 * - **k_fd_table**: A table of file descriptors with individual state.
 *
 * The implementation supports:
 * - Reference-counted file descriptors for sharing via dup/fork.
 * - Both PennFAT files and host OS files.
 * - Lazy block allocation for writes in the FAT filesystem.
 * - Process-local to kernel-global descriptor mapping.
 *
 * @note All functions that modify state assert they are not called from
 *       interrupt context using ASSERT_NO_INTERRUPT.
 */

#include "k_filetable.h"
#include "filetable.h"

#include "io/file/h_file.h"

#include "io/fat/k_fat.h"
#include "process/pcb/k_pcb.h"
#include "process/schedule/k_schedule.h"

#include "io/term/s_term.h"

#include <string.h>
#include "util/vector.h"

/** @brief Global table of open files in the system. */
static vector(k_open_file_entry_t) k_active_files = NULL;

/** @brief Global kernel file descriptor table. */
static vector(k_fd_entry_t) k_fd_table = NULL;

/**
 * @brief Initialize the kernel file descriptor table.
 *
 * @return 0 on success.
 */
int k_fd_init() {
  if (k_fd_table != NULL)
    return 0;
  k_active_files = vector_new(k_open_file_entry_t, 64, NULL);
  k_fd_table = vector_new(k_fd_entry_t, 64, NULL);
  return 0;
}

/**
 * @brief Free the kernel file descriptor table.
 */
void k_fd_clean() {
  vector_free(&k_active_files);
  vector_free(&k_fd_table);
}

/**
 * @brief Allocate an entry in the open file table.
 *
 * Allocate an entry in the open file table. If the file is already open,
 * returns the existing index.
 *
 * @param file The file to add to the open file table.
 * @return Index into k_active_files.
 */
int file_alloc(file_t file) {
  k_open_file_entry_t entry = {.file = file, .refcount = 0};
  if (k_fd_table == NULL)
    k_fd_init();

  int pos = 0;
  int insert_pos = -1;
  /* Search for the file in the list, also find the first empty slot */
  while (pos < vector_len(&k_active_files)) {
    if (k_active_files[pos].refcount) {
      if (strcmp(k_active_files[pos].file.name, file.name) == 0) {
        /* Match found, return entry */
        return pos;
      }
    } else if (insert_pos == -1) {
      insert_pos = pos;
    }
    pos++;
  }

  /* If the file is not found, create an entry */
  if (insert_pos == -1) {
    insert_pos = vector_len(&k_active_files);
    vector_push(&k_active_files, entry);
  } else {
    k_active_files[insert_pos] = entry;
  }
  return insert_pos;
}

/**
 * @brief Allocate an entry in the file descriptor table.
 *
 * Allocate an entry in the open file table. Will always allocate the lowest
 * available file descriptor
 *
 * @param entry The file descriptor entry to insert.
 * @return Index into k_fd_table (the new fd number).
 */
int fd_alloc(k_fd_entry_t entry) {
  if (k_fd_table == NULL)
    k_fd_init();

  /* Find the first empty position */
  int insert_pos = 0;
  while (insert_pos < vector_len(&k_fd_table) &&
         k_fd_table[insert_pos].refcount) {
    insert_pos++;
  }

  /* Claim or create the fd entry */
  if (insert_pos == vector_len(&k_fd_table)) {
    vector_push(&k_fd_table, entry);
  } else {
    k_fd_table[insert_pos] = entry;
  }
  return insert_pos;
}

/**
 * @brief Allocate a kernel file descriptor for a file.
 *
 * Creates a new file descriptor entry for the given file with the specified
 * access mode.
 *
 * @pre Interrupts are disabled
 * @param file The file to open.
 * @param mode Access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC).
 * @return The allocated kernel file descriptor index.
 */
int k_fd_alloc(file_t file, int mode) {
  ASSERT_NO_INTERRUPT("k_fd_alloc");

  /* Obtain a file entry */
  int file_idx = file_alloc(file);
  k_active_files[file_idx].refcount++;

  /* Create entry and forward call to fd_alloc*/
  k_fd_entry_t entry;
  entry.open_file_idx = file_idx;
  entry.refcount = 1;
  entry.mode = mode;
  entry.curr_block = file.first_block.next;
  entry.virtual_offset = entry.target_offset = 0;
  entry.hfd = -1;
  return fd_alloc(entry);
}

/**
 * @brief Allocate a kernel file descriptor for a host fd.
 *
 * Creates a new file descriptor entry that wraps a host fd.
 *
 * @pre Interrupts are disabled
 * @param hfd The host OS file descriptor.
 * @param mode Access mode (F_READ, F_WRITE, etc.).
 * @return The allocated kernel file descriptor index.
 */
int k_fd_alloc_h(int hfd, int mode) {
  ASSERT_NO_INTERRUPT("k_fd_alloc_h");

  /* Create entry and forward call to fd_alloc*/
  k_fd_entry_t entry;
  entry.hfd = hfd;
  entry.open_file_idx = -1;
  entry.mode = mode;
  entry.refcount = 1;
  return fd_alloc(entry);
}

/**
 * @brief Check if a file descriptor entry exists and is valid.
 *
 * @param fd The kernel file descriptor.
 * @return true if the descriptor is valid and has a positive refcount.
 */
bool k_fd_has_entry(int fd) {
  return k_fd_table != NULL && fd >= 0 &&
         (size_t)fd < vector_len(&k_fd_table) && k_fd_table[fd].refcount > 0;
}

// Helper macro for verifying a fd is a valid kernel file descriptor
#ifdef DEBUG
#define ASSERT_HAS_FD(fd, func_name)                  \
  ({                                                  \
    if (!k_fd_has_entry(fd)) {                        \
      s_perror("%s: invalid fd provided", func_name); \
      abort();                                        \
    }                                                 \
  })
#else
#define ASSERT_HAS_FD(fd, func_name) (void)0
#endif

/**
 * @brief Get the host file descriptor if this is a host file.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The host file descriptor if it is a host file, else -1
 */
int k_fd_get_hfd(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_get_hfd");
  ASSERT_HAS_FD(fd, "k_fd_get_hfd");
  return k_fd_table[fd].hfd;
}

/**
 * @brief Duplicate a kernel file descriptor.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor to duplicate.
 * @return The same file descriptor on success.
 */
int k_fd_dup(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_dup");
  ASSERT_HAS_FD(fd, "k_fd_dup");
  k_fd_table[fd].refcount++;
  return fd;
}

/**
 * @brief Close a kernel file descriptor.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor to close.
 * @return 0 on success.
 */
int k_fd_close(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_close");
  ASSERT_HAS_FD(fd, "k_fd_close");
  k_fd_entry_t* entry = k_fd_table + fd;
  if (entry->refcount > 0) {
    entry->refcount--;
    if (entry->refcount == 0) {
      /* Clean up fd if reference count gets to zero*/
      if (entry->hfd != -1) {
        h_close(entry->hfd);
      } else {
        k_active_files[entry->open_file_idx].refcount--;
      }
    }
  }
  return 0;
}

/**
 * @brief Apply pending offset changes to a file descriptor.
 *
 * @param fd The file descriptor to update.
 * @return 0 on success, -1 on error (EOF in read mode or allocation failure).
 */
int apply_offset(int fd) {
  k_fd_entry_t* entry = k_fd_table + fd;
  file_t* file = &k_active_files[entry->open_file_idx].file;

  /* Claim a block if the file doesn't have one */
  if (file->first_block.next == 0xFFFF) {
    if (entry->mode == F_WRITE || entry->mode == F_APPEND) {
      k_start_block(file);
    } else {
      return -1;
    }
  }

  /* Refresh virtual offset when the file gets its first block */
  if (entry->curr_block == 0xFFFF) {
    entry->curr_block = file->first_block.next;
    entry->virtual_offset = 0;
  }

  /* Only walk if we need to */
  if (entry->target_offset == entry->virtual_offset)
    return 0;

  size_t blk_size = k_get_blk_size();
  int target_block = entry->target_offset / blk_size;

  /* delta is the number of blocks we need to walk forward */
  int delta = target_block - entry->virtual_offset / blk_size;

  /* Reset virtual pointer to start if block is behind it */
  if (delta < 0) {
    entry->virtual_offset = 0;
    entry->curr_block = file->first_block.next;
    delta = target_block - entry->virtual_offset / blk_size;
  }
  if (delta < 0) {
    s_perror("apply_offset: critical file pointer error\n");
  }

  /* Walk forward the necessary number of blocks */
  for (int i = 0; i < delta; i++) {
    fat_t* curr = k_get_fat(entry->curr_block);

    /* Create a new entry if we are writing */
    if (curr->next == 0xFFFF) {
      if (entry->mode == F_WRITE || entry->mode == F_APPEND) {
        curr->next = k_request_block();
        if (curr->next == 0xFFFF) {
          return -1;
        }
      } else {
        return -1;
      }
    }

    /* Go to next block */
    entry->curr_block = curr->next;
  }

  entry->virtual_offset = entry->target_offset;
  return 0;
}

/**
 * @brief Walk through file blocks for I/O operations.
 *
 * Advances through the file's FAT chain, returning the position and size
 * of the next chunk to read/write.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @param[in,out] target_size Remaining bytes to process; updated after call.
 * @param[out] pos Output: position in the data region for this chunk.
 * @param[out] size Output: number of bytes available in this chunk.
 * @return 0 on success, -1 on error.
 */
int k_fd_walk(int fd, size_t* target_size, int* pos, size_t* size) {
  ASSERT_NO_INTERRUPT("k_fd_walk");
  ASSERT_HAS_FD(fd, "k_fd_walk");
  k_fd_entry_t* file = k_fd_table + fd;
  size_t blk_size = k_get_blk_size();
  size_t curr_offset = file->target_offset % blk_size;
  size_t rem_blk = blk_size - curr_offset;

  /* Ensure curr_block is the correct fat block */
  if (apply_offset(fd) == -1)
    return -1;

  if (rem_blk < *target_size) {
    /* The goal size is greater than the amount we can give */
    *pos = k_get_data_pos(file->curr_block, curr_offset);
    *size = rem_blk;
    *target_size -= rem_blk;
    file->target_offset += rem_blk;
  } else {
    /* The goal size is achievable */
    *pos = k_get_data_pos(file->curr_block, curr_offset);
    *size = *target_size;
    file->target_offset += *target_size;
    *target_size = 0;
  }
  return 0;
}

/**
 * @brief Get the current file offset.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The current target offset in bytes.
 */
uint32_t k_fd_get_offset(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_get_offset");
  ASSERT_HAS_FD(fd, "k_fd_get_offset");
  return k_fd_table[fd].target_offset;
}

/**
 * @brief Set the file offset.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @param off The new offset in bytes.
 */
void k_fd_set_offset(int fd, int off) {
  ASSERT_NO_INTERRUPT("k_fd_set_offset");
  ASSERT_HAS_FD(fd, "k_fd_set_offset");
  k_fd_table[fd].target_offset = off;
}

/**
 * @brief Get the size of the file.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return The file size in bytes.
 */
int k_fd_get_size(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_get_size");
  ASSERT_HAS_FD(fd, "k_fd_get_size");
  return k_active_files[k_fd_table[fd].open_file_idx].file.size;
}

/**
 * @brief Get the access mode of a file descriptor.
 *
 * @param fd The kernel file descriptor.
 * @return The access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC).
 */
int k_fd_mode(int fd) {
  ASSERT_HAS_FD(fd, "k_fd_mode");
  return k_fd_table[fd].mode;
}

/**
 * @brief Get a reference to the file referenced by a fd.
 *
 * @pre Interrupts are disabled
 * @param fd The kernel file descriptor.
 * @return Pointer to the file_t structure.
 */
file_t* k_fd_file_ref(int fd) {
  ASSERT_NO_INTERRUPT("k_fd_file_ref");
  ASSERT_HAS_FD(fd, "k_fd_file_ref");
  if (k_fd_table[fd].open_file_idx == -1) {
    return NULL;
  }
  return &k_active_files[k_fd_table[fd].open_file_idx].file;
}

/**
 * @brief Add a kernel file descriptor to a process's local table.
 *
 * Maps a global kernel fd to a process-local fd number.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param hfd The kernel file descriptor to add.
 * @return The local file descriptor number assigned.
 */
int k_lfd_put(int pid, int hfd) {
  ASSERT_NO_INTERRUPT("k_lfd_put");
  vector(int)* lfd_table = &k_pcb_data(pid)->fd_table;
  int insert_pos = 0;
  while (insert_pos < vector_len(lfd_table) && (*lfd_table)[insert_pos] != -1) {
    insert_pos++;
  }
  if (insert_pos == vector_len(lfd_table)) {
    vector_push(lfd_table, hfd);
  } else {
    (*lfd_table)[insert_pos] = hfd;
  }
  return insert_pos;
}

/**
 * @brief Release a local file descriptor from a process.
 *
 * Removes the mapping and closes the underlying kernel fd.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param fd The local file descriptor to release.
 * @return 0 on success, -1 if invalid.
 */
int k_lfd_release(int pid, int fd) {
  ASSERT_NO_INTERRUPT("k_lfd_release");
  int gfd = k_pcb_data(pid)->fd_table[fd];
  if (gfd == -1) {
    return -1;
  }
  ASSERT_HAS_FD(fd, "k_lfd_release");
  k_pcb_data(pid)->fd_table[fd] = -1;
  k_fd_close(gfd);
  return 0;
}

/**
 * @brief Get the kernel file descriptor from a local descriptor.
 *
 * Translates a process-local fd to the global kernel fd.
 *
 * @pre Interrupts are disabled
 * @param pid The process ID.
 * @param fd The local file descriptor.
 * @return The kernel file descriptor, or -1 if invalid.
 */
int k_lfd_get(int pid, int fd) {
  ASSERT_NO_INTERRUPT("k_lfd_get");
  vector(int)* fd_table = &k_pcb_data(pid)->fd_table;
  if (fd < 0 || fd >= vector_len(fd_table))
    return -1;
  return (*fd_table)[fd];
}

/**
 * @brief Check if a file is currently open.
 *
 * @pre Interrupts are disabled
 * @param fname The filename to check.
 * @return 1 if the file is open, 0 otherwise.
 */
int k_fd_is_open(const char* fname) {
  ASSERT_NO_INTERRUPT("k_fd_is_open");
  if (k_fd_table == NULL) {
    return 0;
  }

  for (size_t i = 0; i < vector_len(&k_active_files); i++) {
    k_open_file_entry_t* entry = &k_active_files[i];
    if (entry->refcount > 0 && strcmp(entry->file.name, fname) == 0) {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Check if a file is open with write or append mode.
 *
 * Used to enforce write locking - only one writer allowed at a time.
 *
 * @pre Interrupts are disabled
 * @param fname The filename to check.
 * @return 1 if the file is write-locked, 0 otherwise.
 */
int k_fd_is_write_locked(const char* fname) {
  ASSERT_NO_INTERRUPT("k_fd_is_write_locked");
  if (k_fd_table == NULL) {
    return 0;
  }

  for (size_t i = 0; i < vector_len(&k_fd_table); i++) {
    k_fd_entry_t* entry = &k_fd_table[i];

    if (entry->refcount > 0 && entry->open_file_idx != -1 &&
        (entry->mode == F_WRITE || entry->mode == F_APPEND) &&
        strcmp(k_active_files[entry->open_file_idx].file.name, fname) == 0) {
      return 1;
    }
  }
  return 0;
}
