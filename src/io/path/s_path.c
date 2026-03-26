/**
 * @file s_path.c
 * @brief Implementation of system-level directory listing.
 *
 * This file implements the s_ls() system call for listing directory
 * contents in PennOS. It formats and prints file entries with their
 * metadata including permissions, size, modification time, and name.
 */

#include "s_path.h"

#include "io/fat/file_t.h"
#include "io/file/k_file.h"
#include "io/filetable/k_filetable.h"
#include "io/fstd.h"
#include "io/term/s_term.h"
#include "k_path.h"
#include "process/schedule/k_schedule.h"

#include <string.h>

/**
 * @brief Print a formatted file entry to the terminal.
 *
 * Displays file information in the format:
 * `[block] - [rwx] [size] [mtime] [name]`
 *
 * Where block is the first data block number (or "-" if none allocated),
 * rwx are the permission flags, size is in bytes, mtime is the
 * modification timestamp, and name is the filename.
 *
 * @param file The file entry to print.
 */
void print_file_entry(file_t file) {
  char permission_str[4];
  permission_str[0] = (file.perm & 4) ? 'r' : '-';
  permission_str[1] = (file.perm & 2) ? 'w' : '-';
  permission_str[2] = (file.perm & 1) ? 'x' : '-';
  permission_str[3] = '\0';

  char* time_str = ctime(&file.mtime);
  time_str[strlen(time_str) - 1] = '\0';

  if (file.first_block.next == 0xFFFF) {
    s_printf("    - %s %6u %s %s\n", permission_str, (unsigned)file.size,
             time_str, file.name);
  } else {
    s_printf("%3u - %s %6u %s %s\n", (unsigned)file.first_block.next,
             permission_str, (unsigned)file.size, time_str, file.name);
  }
}

/**
 * @brief List files in the root directory.
 *
 * Disables interrupts during execution for consistency.
 * If a filename is provided, searches for and displays only that file.
 * Otherwise, iterates through all directory entries and prints each one.
 *
 * @param filename The specific file to list, or NULL to list all files.
 * @return 0 on success, -1 on error (e.g., cannot open directory).
 */
int s_ls(const char* filename) {
  k_disable_interrupts();
  file_t file;
  int idx =
      (filename == NULL) ? -1 : k_dir_search(filename, k_get_root_dir(), &file);
  if (idx != -1) {
    print_file_entry(file);
    return 0;
  }

  int fd = k_fd_alloc(*k_get_root_dir(), F_READ);
  if (fd < 0)
    return -1;

  /* Iterate entries */
  file_t curr_file;
  while (k_read(fd, (char*)&curr_file, sizeof(file_t)) == sizeof(file_t)) {
    if (curr_file.name[0] == '\0')
      break;
    print_file_entry(curr_file);
  }
  k_fd_close(fd);
  k_enable_interrupts();
  return 0;
}