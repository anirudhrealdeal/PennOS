/**
 * @file k_path.c
 * @brief Implementation of kernel-level directory operations.
 *
 * This file implements directory management for the PennFAT filesystem.
 * It provides functions to search, add, update, and remove directory entries.
 * Directory entries are stored as sequential file_t structures within the
 * directory's data blocks.
 *
 * @note The root directory is a special file_t with type=2 and first_block=1.
 *       All file operations on directories use the same k_read/k_write
 *       infrastructure as regular files.
 */

#include "k_path.h"

#include "s_path.h"

#include "io/fat/k_fat.h"
#include "io/file/k_file.h"
#include "io/filetable/k_filetable.h"
#include "io/fstd.h"
#include "io/term/s_term.h"
#include "process/schedule/k_schedule.h"

#include <stdio.h>
#include <string.h>

/** @brief The global root directory of the PennFAT filesystem. */
file_t root_dir = (file_t){.name = "", .type = 2, .first_block = {1}};

/**
 * @brief Get a pointer to the root directory.
 *
 * @return Pointer to the root directory file_t structure.
 */
file_t* k_get_root_dir() {
  return &root_dir;
}

/**
 * @brief Search for a file in a directory.
 *
 * Opens the directory for reading and iterates through all entries
 * until a match is found or the end is reached (null-terminated name).
 *
 * @param fname The filename to search for.
 * @param dir The directory to search in.
 * @param[out] file Output: the file entry if found (can be NULL).
 * @return The index of the file entry if found, -1 otherwise.
 */
int k_dir_search(const char* fname, file_t* dir, file_t* file) {
  ASSERT_NO_INTERRUPT("k_dir_search");
  if (!k_has_mount())
    return -1;

  int fd = k_fd_alloc(*dir, F_READ);
  if (fd < 0) {
    return -1;
  }

  /* Iterate entries */
  int idx = 0;
  file_t curr_file;
  int nbytes;
  while ((nbytes = k_read(fd, (char*)&curr_file, sizeof(file_t))) ==
         sizeof(file_t)) {
    if (curr_file.name[0] == '\0')
      break;
    if (strncmp(curr_file.name, fname, sizeof(curr_file.name)) == 0) {
      if (file)
        *file = curr_file;
      k_fd_close(fd);
      return idx;
    }
    idx++;
  }

  k_fd_close(fd);
  return -1;
}

/**
 * @brief Update a directory entry at a given index.
 *
 * Opens the directory for writing, seeks to the entry position,
 * and overwrites it with the new file data.
 *
 * @param dir The directory to update.
 * @param idx The index of the entry to update.
 * @param file The new file entry data.
 */
void k_dir_upd_entry(file_t* dir, int idx, file_t file) {
  ASSERT_NO_INTERRUPT("k_dir_upd_entry");
  if (!k_has_mount())
    return;

  int fd = k_fd_alloc(*dir, F_WRITE);
  if (fd < 0)
    return;

  int pos = idx * (int)sizeof(file_t);
  k_lseek(fd, pos, F_SEEK_SET);
  k_write(fd, (const char*)&file, sizeof(file_t));

  k_fd_close(fd);
}

/**
 * @brief Remove a directory entry at a given index.
 *
 * Uses two file descriptors: one for reading subsequent entries
 * and one for writing them back shifted by one position.
 * Terminates with a zeroed entry.
 *
 * @param dir The directory to modify.
 * @param idx The index of the entry to remove.
 */
void k_dir_rem_entry(file_t* dir, int idx) {
  ASSERT_NO_INTERRUPT("k_dir_rem_entry");
  if (!k_has_mount())
    return;

  int fd = k_fd_alloc(*dir, F_WRITE);
  int fd2 = k_fd_alloc(*dir, F_READ);
  if (fd < 0)
    return;

  k_lseek(fd, idx * sizeof(file_t), F_SEEK_SET);
  k_lseek(fd2, (idx + 1) * sizeof(file_t), F_SEEK_SET);

  file_t curr_file;
  while (k_read(fd2, (char*)&curr_file, sizeof(file_t)) == sizeof(file_t)) {
    if (curr_file.name[0] == '\0')
      break;
    if (k_write(fd, &curr_file, sizeof(file_t)) != sizeof(file_t)) {
      s_perror("k_dir_rem_entry: bad write in directory removal\n");
    }
  }
  memset(&curr_file, 0, sizeof(file_t));
  k_write(fd, &curr_file, sizeof(file_t));
  k_fd_close(fd);
  k_fd_close(fd2);
}

/**
 * @brief Add a new entry to a directory.
 *
 * Scans through the directory to find the end (first null entry),
 * then writes the new file entry. Updates directory size if needed.
 * Ensures proper null termination for block boundary alignment.
 *
 * @param dir The directory to add the entry to.
 * @param file The file entry to add.
 * @return 0 on success, -1 on error.
 */
int k_dir_add_entry(file_t* dir, file_t file) {
  ASSERT_NO_INTERRUPT("k_dir_add_entry");
  if (!k_has_mount())
    return -1;

  int fd = k_fd_alloc(*dir, F_WRITE);
  if (fd < 0)
    return -1;

  int idx = 0;
  file_t curr_file;
  while (k_read(fd, (char*)&curr_file, sizeof(file_t)) == sizeof(file_t)) {
    if (curr_file.name[0] == '\0') {
      break;
    }
    idx++;
  }

  k_lseek(fd, idx * sizeof(file_t), F_SEEK_SET);
  if (k_write(fd, &file, sizeof(file_t)) < 0) {
    k_fd_close(fd);
    return -1;
  }

  size_t sz = (idx + 1) * sizeof(file_t);

  if ((idx + 1) % (k_get_blk_size() / sizeof(file_t)) != 0) {
    char zero = '\0';
    if (k_write(fd, &zero, 1) < 0) {
      k_fd_close(fd);
      return -1;
    }
    sz++;
  }

  if (dir->size < sz) {
    dir->size = sz;
  }

  k_fd_close(fd);
  return 0;
}

/**
 * @brief List directory contents (alias for s_ls).
 *
 * Required by PennOS specification. Delegates to s_ls() for implementation.
 *
 * @param filename The specific file to list, or NULL for all files.
 * @return 0 on success, -1 on error.
 */
int k_ls(const char* filename) {
  return s_ls(filename);
}
