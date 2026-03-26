/**
 * @file pennfat.h
 * @brief Header for the PennFAT standalone filesystem utility.
 *
 * This header provides declarations for the PennFAT filesystem interface.
 * The actual filesystem operations are implemented in the io/fat/ modules.
 *
 * @note Most FAT operations have been modularized into separate files:
 * - io/fat/k_fat.h: Kernel-level FAT operations
 * - io/fat/s_fat.h: System-level FAT wrappers
 * - io/file/k_file.h: File I/O operations
 * - shell/file/u_file.h: User-level file commands
 */

#ifndef PENNFAT_H
#define PENNFAT_H

// #include <stdbool.h>
// #include <stdint.h>
// #include <time.h>
// #include "util/vector/vector.h"

// Constants
// extern const uint16_t F_WRITE;
// extern const uint16_t F_READ;
// extern const uint16_t F_APPEND;

// Basic file structure for fat filesystem
// typedef struct file {
//   char name[32];
//   uint32_t size;
//   uint16_t first_block;
//   uint8_t type;
//   uint8_t perm;
//   time_t mtime;
// } file_t;

// FAT stucture
// typedef struct fat {
//   uint8_t msb;
//   uint8_t lsb;
//   vector(file_t) root;
// } fat_t;

// Helper Functions
// int k_get_block_size(int block_size_config);

// K Level Functions
// char* k_ls(const char* filename);
// int k_open(const char* fname, int mode);
// int k_read(int fd, int n, char* buf);
// int k_write(int fd, const char* str, int n);
// int k_close(int fd);
// int k_unlink(const char* fname);
// void k_lseek(int fd, int offset, int whence);

// S Level Functions
// int s_open(const char* fname, int mode);
// int s_read(int fd, int n, char* buf);
// int s_write(int fd, const char* str, int n);
// int s_close(int fd);
// int s_unlink(const char* fname);
// void s_lseek(int fd, int offset, int whence);
// vector(file_t) s_ls(const char* filename);
// int s_rename_file(const char* source, const char* dest);

// U Level Functions
// void u_ls(const char* filename);
// void u_touch(char** args);
// void u_mv(char** args);
// void u_rm(char** args);
// void u_cat(char** args);
// void u_cp(char** args);
// void u_chmod(char** args);

#endif