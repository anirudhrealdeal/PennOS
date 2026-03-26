/**
 * @file k_file.c
 * @brief Implementation of kernel file I/O operations
 *
 * Implements file operations for a custom FAT-based filesystem including
 * file creation, deletion, reading, writing, and metadata management.
 * Handles both internal filesystem files and external host files.
 */

#include "k_file.h"

#include <string.h>
#include <unistd.h>
#include "io/fat/k_fat.h"
#include "io/file/h_file.h"
#include "io/filetable/k_filetable.h"
#include "io/path/k_path.h"
#include "process/schedule/k_schedule.h"

/**
 * @brief Opens a file in the FAT filesystem in the provided mode
 *
 * Opens a file using the provided mode.
 *
 * The modes work as follows:
 *  - F_READ / F_EXEC = Readonly access to file, errors if doesn't exist.
 *  - F_WRITE = Write access to file, truncates the file if exists.
 *  - F_APPEND = Write access to file, subsequent writes extend the file.
 *
 * Will error if the file exists and has the wrong permission. Only one open can
 * have write capability to a file at a time, subsequent opens will error.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the file in the filesystem
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Kernel file descriptor on success, -1 on error
 * @note Emits perror messages upon error with the relevant information
 */
int k_open(const char* fname, int mode) {
  ASSERT_NO_INTERRUPT("k_open");
  if (!k_has_mount())
    return -1;

  /* Error if attempting double write access open */
  if ((mode == F_WRITE || mode == F_APPEND) && k_fd_is_write_locked(fname)) {
    s_perror("%s: file already being written to\n", fname);
    return -1;
  }

  file_t file;
  int idx = k_dir_search(fname, k_get_root_dir(), &file);
  if (idx == -1) {
    /* The file doesn't exist, create if correct mode */
    if (mode == F_WRITE || mode == F_APPEND) {
      /* Adds file to root directory, creating the file */
      file = (file_t){.size = 0,
                      .first_block = {0xFFFF},
                      .type = 1,
                      .perm = 6,
                      .mtime = time(NULL)};
      strncpy(file.name, fname, 31);
      if (k_dir_add_entry(k_get_root_dir(), file) == -1) {
        s_perror("%s: failed to add to directory\n", fname);
        return -1;
      }
    } else {
      s_perror("%s: file does not exist\n", fname);
      return -1;
    }
  } else {
    /* The file exists, check permissions */
    if (mode == F_EXEC && !(file.perm & 1)) {
      s_perror("%s: not executable\n", fname);
    } else if (mode == F_READ && !(file.perm & 4)) {
      s_perror("%s: no read permission\n", fname);
      return -1;
    } else if ((mode == F_WRITE || mode == F_APPEND) && !(file.perm & 2)) {
      s_perror("%s: no write permission\n", fname);
      return -1;
    }
  }

  /* Create a file descriptor for this file */
  int kfd = k_fd_alloc(file, mode);

  if (idx != -1) {
    if (mode == F_WRITE) {
      /* Truncate files if exist */
      file_t* file = k_fd_file_ref(kfd);
      k_release_fat_chain(file->first_block.next);

      /* Reflect truncation in file entry */
      file->size = 0;
      file->first_block.next = 0xFFFF;
      file->mtime = time(NULL);
      k_dir_upd_entry(k_get_root_dir(), idx, *file);
    }
    if (mode == F_APPEND) {
      /* Move write pointer to EOF */
      file_t* file = k_fd_file_ref(kfd);

      /* Update modification time in file entry */
      file->mtime = time(NULL);
      k_dir_upd_entry(k_get_root_dir(), idx, *file);
      k_lseek(kfd, 0, SEEK_END);
    }
  }
  return kfd;
}

/**
 * @brief Opens an external host file.
 *
 * Opens a file from the host filesystem and registers it in the
 * kernel file descriptor table for unified access.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the external file
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Kernel file descriptor on success, -1 on error
 * @see h_open for how modes are handled
 */
int k_ext_open(const char* fname, int mode) {
  ASSERT_NO_INTERRUPT("k_ext_open");

  /* Grab host file descriptor */
  int fd = h_open(fname, mode);
  if (fd == -1)
    return -1;

  /* Create and return kernel file descriptor entry */
  return k_fd_alloc_h(fd, mode);
}

/**
 * @brief Reads data from a kernel file descriptor for a local fs file.
 *
 * @pre fd references a local file
 * @pre Interrupts are disabled
 * @param fd File descriptor to read from
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error
 * @see standard read() function for details
 * @see k_ext_read for reading from host file descriptors
 */
int k_read(int fd, void* buf, size_t nbytes) {
  ASSERT_NO_INTERRUPT("k_read");

  /* Get file entry */
  file_t* meta = k_fd_file_ref(fd);
  if (!meta)
    return -1;

  /* Truncate read based on file limits */
  size_t current_pos = k_fd_get_offset(fd);
  if (current_pos >= meta->size)
    return 0;
  size_t bytes_available = meta->size - current_pos;
  if (nbytes > bytes_available)
    nbytes = bytes_available;

  int pos;
  size_t target_size = nbytes, size;
  int offset = 0;
  while (target_size) {
    /* k_fd_walk tells us the position and size of our next read, walking
     * forward the file offset to prepare for next read */
    if (k_fd_walk(fd, &target_size, &pos, &size) == -1) {
      break;
    }

    /* Read from the provided memory location using the filesystem fd */
    lseek(k_get_fat_fd(), pos, SEEK_SET);
    int ret = read(k_get_fat_fd(), (char*)buf + offset, size);
    if (ret <= 0) {
      return ret;
    }

    offset += (uint32_t)ret;
  }

  /* At this point, offset == total size read */
  return (int)offset;
}

/**
 * @copydoc h_read
 */
int k_ext_read(int fd, void* buf, size_t nbytes) {
  return h_read(fd, buf, nbytes);
}

/**
 * @brief Writes data to a kernel file descriptor
 *
 * @pre Interrupts are disabled
 * @param fd Kernel file descriptor
 * @param buf Buffer containing data to write
 * @param nbytes Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
int k_write(int fd, const void* buf, size_t nbytes) {
  ASSERT_NO_INTERRUPT("k_write");
  /* Redirect to h_write if host fd */
  k_disable_interrupts();
  int hfd = k_fd_get_hfd(fd);
  k_enable_interrupts();
  if (hfd != -1) {
    return h_write(hfd, buf, nbytes);
  }

  int pos;
  size_t target_size = nbytes, size;
  int offset = 0;
  while (target_size) {
    /* k_fd_walk tells us the position and size of our next read, walking
     * forward the file offset to prepare for next read */
    if (k_fd_walk(fd, &target_size, &pos, &size) == -1)
      return -1;

    /* Write to the provided memory location using the filesystem fd */
    lseek(k_get_fat_fd(), pos, SEEK_SET);
    int ret = write(k_get_fat_fd(), (char*)buf + offset, size);
    if (ret < 0)
      return -1;

    /* If we are within a page size of the MMAP region, flushing is handled
     * differently. Double write to ensure ouput is properly flushed to file */
    if (pos <= k_sizeof_fat(*k_get_fat(0)) + 4096) {
      lseek(k_get_fat_fd(), pos, SEEK_SET);
      ret = write(k_get_fat_fd(), (char*)buf + offset, size);
      if (ret < 0)
        return -1;
    }

    offset += ret;
  }

  /* Update the total size, modification time of the file */
  file_t* meta = k_fd_file_ref(fd);
  if (meta) {
    int idx = k_dir_search(meta->name, k_get_root_dir(), meta);
    if (idx >= 0) {
      uint32_t new_off = (uint32_t)k_fd_get_offset(fd);
      if (new_off > meta->size)
        meta->size = new_off;
      meta->mtime = time(NULL);
      k_dir_upd_entry(k_get_root_dir(), idx, *meta);
    }
  }

  return (int)offset;
}

/**
 * @copydoc k_fd_close
 */
int k_close(int fd) {
  return k_fd_close(fd);
}

/**
 * @brief Deletes a file from the filesystem
 *
 * Deletes a file from the filesystem. Fails if the file is currently open by
 * any process.
 *
 * @pre Interrupts are disabled
 * @param fname Path to the file to delete
 * @return 0 on success, -1 on error (file open or not found)
 */
int k_unlink(const char* fname) {
  ASSERT_NO_INTERRUPT("k_unlink");
  /* Verify file isn't open */
  if (k_fd_is_open(fname)) {
    return -1;
  }

  /* Verify file exists */
  file_t file;
  int pos = k_dir_search(fname, k_get_root_dir(), &file);
  if (pos == -1)
    return -1;

  /* Delete the file */
  k_dir_rem_entry(k_get_root_dir(), pos);
  k_release_fat_chain(file.first_block.next);
  return 0;
}

/**
 * @brief Sets the file offset for the given file descriptor
 *
 * Sets the file offset for the given file descriptor. The whence operator acts
 * the same as whence for standard lseek on the subset of options F_SEEK_SET,
 * F_SEEK_CUR, and F_SEEK_END.
 *
 * @pre Interrupts are disabled
 * @param fd Kernel file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 */
void k_lseek(int fd, int offset, int whence) {
  ASSERT_NO_INTERRUPT("k_lseek");
  /* Redirect to host */
  k_disable_interrupts();
  int hfd = k_fd_get_hfd(fd);
  k_enable_interrupts();
  if (hfd != -1) {
    return h_lseek(fd, offset, whence);
  }

  /* Call to change the offset, specifics TBD by whence */
  if (whence == F_SEEK_SET) {
    k_fd_set_offset(fd, offset);
  } else if (whence == F_SEEK_CUR) {
    k_fd_set_offset(fd, k_fd_get_offset(fd) + offset);
  } else if (whence == F_SEEK_END) {
    k_fd_set_offset(fd, k_fd_get_size(fd));
  } else {
    s_perror("k_lseek: invalid whence\n");
  }
}

/**
 * @brief Renames a file in the filesystem
 *
 * Renames a file to the new name, replacing the destination file if it exists.
 * Will error if the source file doesn't exist, if either file is open, or if
 * the source and destination are the same.
 *
 * @pre Interrupts are disabled
 * @param source Current file name
 * @param dest New file name
 * @return 0 on success, -1 on error (file open, not found, or cannot add)
 */
int k_mv(const char* source, const char* dest) {
  ASSERT_NO_INTERRUPT("k_mv");
  if (!k_has_mount()) {
    return -1;
  }

  /* Break early if source and dest are the same */
  if (strcmp(source, dest) == 0) {
    return -1;
  }

  /* Verify files aren't open */
  if (k_fd_is_open(source)) {
    return -1;
  }
  if (k_fd_is_open(dest)) {
    return -1;
  }

  /* Verify source file exists */
  file_t source_file;
  int search_res = k_dir_search(source, k_get_root_dir(), &source_file);
  if (search_res < 0) {
    return -1;
  }

  /* Delete destination file. This may error because it doesn't exist, but that
   * doesn't matter in this case */
  k_unlink(dest);

  /* Find the source file again since its position in the directory may have
   * changed after the destination was removed */
  search_res = k_dir_search(source, k_get_root_dir(), &source_file);
  if (search_res < 0) {
    return -1;
  }

  /* Rename the file entry, update modification time */
  strncpy(source_file.name, dest, 31);
  source_file.name[31] = '\0';
  source_file.mtime = time(NULL);

  /* Replace our file entry with the updated name */
  k_dir_rem_entry(k_get_root_dir(), search_res);
  int add_res = k_dir_add_entry(k_get_root_dir(), source_file);
  if (add_res < 0) {
    return -1;
  }
  return 0;
}
