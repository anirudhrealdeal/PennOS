/**
 * @file pennfat.c
 * @brief Standalone PennFAT filesystem utility.
 *
 * This file implements a standalone command-line interface for interacting
 * with PennFAT filesystems. It provides commands for creating, mounting,
 * and manipulating FAT filesystems outside of PennOS.
 *
 * Supported commands:
 * - mkfs: Create a new filesystem
 * - mount/unmount: Mount/unmount a filesystem
 * - ls, touch, cat, chmod, rm, mv, cp: File operations
 */

#include "pennfat.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "io/fat/s_fat.h"
#include "io/filetable/k_filetable.h"
#include "io/term/s_term.h"
#include "shell/file/u_file.h"
#include "util/parser.h"
#include "util/vector.h"

/**
 * @brief Main entry point for the PennFAT utility.
 *
 * Runs an interactive REPL loop that accepts filesystem commands.
 * Ignores SIGINT and SIGTSTP to prevent accidental termination.
 * Initializes stdin/stdout/stderr in the kernel file descriptor table.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return 0 on successful exit.
 */
int main(int argc, char* argv[]) {
  // signal handlers
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);

  k_fd_alloc_h(STDIN_FILENO, F_READ);
  k_fd_alloc_h(STDOUT_FILENO, F_APPEND);
  k_fd_alloc_h(STDERR_FILENO, F_APPEND);

  char input[1024];

  while (1) {
    s_printf("pennfat# ");
    fflush(0);

    if (fgets(input, sizeof(input), stdin) == NULL) {
      // ctlr-d
      s_printf("\n");
      break;
    }

    // Parse input of the command
    struct parsed_command* cmd;
    if (parse_command(input, &cmd) != 0) {
      continue;
    }

    if (cmd->commands[0] == NULL) {
      continue;
    }

    char** args = cmd->commands[0];

    // commands to be executed
    if (strcmp(args[0], "mkfs") == 0) {
      if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        s_fprintf(
            STDERR_FILENO,
            "Usage: mkfs <filename> <blocks_in_fat> <block_size_config>\n");
      } else {
        s_mkfs(args[1], atoi(args[2]), atoi(args[3]));
      }
    } else if (strcmp(args[0], "mount") == 0) {
      s_mount(args[1]);
    } else if (strcmp(args[0], "unmount") == 0) {
      s_unmount();
    } else if (!s_has_mount()) {
      s_fprintf(STDERR_FILENO, "No filesystem mounted\n");
    } else if (strcmp(args[0], "ls") == 0) {
      u_ls(args);
    } else if (strcmp(args[0], "touch") == 0) {
      u_touch(args);
    } else if (strcmp(args[0], "cat") == 0) {
      u_cat(args);
    } else if (strcmp(args[0], "chmod") == 0) {
      u_chmod(args);
    } else if (strcmp(args[0], "rm") == 0) {
      u_rm(args);
    } else if (strcmp(args[0], "mv") == 0) {
      u_mv(args);
    } else if (strcmp(args[0], "cp") == 0) {
      u_cp(args);
    } else {
      s_fprintf(STDERR_FILENO, "pennfat command not found: %s\n", args[0]);
    }

    free(cmd);
  }

  if (s_has_mount()) {
    s_unmount();
  }
  k_fd_clean();

  return 0;
}