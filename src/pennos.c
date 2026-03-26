/**
 * @file pennos.c
 * @brief PennOS kernel main entry point and INIT process implementation.
 *
 * This file contains the main() function that boots PennOS and the INIT
 * process that serves as the parent of all user processes. PennOS is a
 * Unix-like operating system with:
 * - Priority-based process scheduling (3 levels)
 * - FAT filesystem support
 * - Job control and signal handling
 * - Interactive shell
 */
#include "pennos.h"

#include "io/fat/s_fat.h"
#include "io/filetable/k_filetable.h"
#include "io/term/s_term.h"
#include "process/lifecycle/k_lifecycle.h"
#include "process/lifecycle/s_lifecycle.h"
#include "process/pcb/k_pcb.h"
#include "process/pcb/s_pcb.h"
#include "process/schedule/k_schedule.h"
#include "process/signal/s_signal.h"
#include "shell/u_shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/log.h"
#include "util/spthread.h"

/** @brief Default log file path if not specified on command line. */
#define DEFAULT_LOG_FILE "log"

/**
 * @brief Main entry point for PennOS.
 *
 * Initializes all kernel subsystems and starts the scheduler.
 * Boot sequence:
 * 1. Initialize standard file descriptors (stdin, stdout, stderr)
 * 2. Parse command line arguments (fatfs file, optional log file)
 * 3. Initialize logging subsystem
 * 4. Mount the FAT filesystem
 * 5. Initialize PCB table
 * 6. Initialize scheduler
 * 7. Spawn INIT process (PID 1)
 * 8. Start scheduler main loop
 * 9. Cleanup and shutdown
 *
 * @param argc Argument count.
 * @param argv Argument vector: argv[1] = FAT filesystem file,
 *             argv[2] = log file (optional, defaults to "log").
 * @return 0 on successful shutdown, 1 on initialization error.
 */
int main(int argc, char* argv[]) {
  int in_fd = k_fd_alloc_h(STDIN_FILENO, F_READ);
  int out_fd = k_fd_alloc_h(STDOUT_FILENO, F_APPEND);
  int err_fd = k_fd_alloc_h(STDERR_FILENO, F_APPEND);

  if (argc < 2) {
    s_fprintf(STDERR_FILENO, "Usage: %s <fatfs> [log_file]\n", argv[0]);
    return 1;
  }

  const char* fatfs_file = argv[1];
  const char* log_file = argc >= 3 ? argv[2] : DEFAULT_LOG_FILE;

  /* Initialize logging */
  if (log_init(log_file) != 0) {
    s_fprintf(STDERR_FILENO, "Error: Failed to initialize logging to %s\n",
              log_file);
    return 1;
  }

  if (s_mount(fatfs_file) < 0) {
    return 1;
  }

  s_printf("PennOS starting...\n");
  s_printf("FAT filesystem: %s\n", fatfs_file);
  s_printf("Log file: %s\n", log_file);

  /* Initialize process control block table */
  if (k_pcb_init() != 0) {
    s_fprintf(STDERR_FILENO, "Error: Failed to initialize PCB table\n");
    log_close();
    return 1;
  }

  /* Initialize scheduler */
  if (k_sched_init() != 0) {
    s_fprintf(STDERR_FILENO, "Error: Failed to initialize scheduler\n");
    log_close();
    return 1;
  }

  pid_t init_pid = s_spawn(u_init_main, "init", NULL, in_fd, out_fd, err_fd);
  s_nice(init_pid, 0);
  if (init_pid != 1) {
    s_fprintf(STDERR_FILENO, "Error: INIT must have PID 1\n");
    log_close();
    return 1;
  }

  extern size_t ORPHAN_FALLBACK;
  ORPHAN_FALLBACK = init_pid;
  s_printf("Starting scheduler...\n");
  /* Start the scheduler - this will not return until shutdown */
  k_sched_run();
  /* Cleanup */
  s_printf("PennOS shutting down...\n");
  k_proc_cleanup(init_pid);
  k_sched_clean();
  k_fd_clean();
  k_pcb_clean();
  s_unmount();
  log_close();
  return 0;
}

/**
 * @brief INIT process - reaps orphaned and zombie children.
 *
 * INIT is the first user process (PID 1) and serves as the ancestor
 * of all other processes. It performs two main functions:
 *
 * 1. **Shell spawning**: Creates the interactive shell process and
 *    configures it to ignore SIGINT/SIGSTOP (so Ctrl-C/Ctrl-Z affect
 *    foreground jobs, not the shell itself).
 *
 * 2. **Zombie reaping**: Continuously calls s_waitpid(-1) to reap
 *    any child that terminates. This includes:
 *    - Direct children (the shell)
 *    - Orphaned processes whose parents exited
 *
 * INIT exits only when the shell terminates (via logout or Ctrl-D),
 * at which point it reaps any remaining zombies and returns.
 *
 * @param arg Unused parameter (required by spthread function signature).
 * @return NULL on exit.
 */
void* u_init_main(void* arg) {
  pid_t shell_pid = s_spawn(u_shell, "shell", NULL, STDIN_FILENO, STDOUT_FILENO,
                            STDERR_FILENO);
  s_ignore_int(shell_pid, true);
  s_ignore_stop(shell_pid, true);
  s_nice(shell_pid, 0);
  while (1) {
    /* Wait for any child to change state */
    int status;
    pid_t pid = s_waitpid(-1, &status, false);
    /* INIT runs indefinitely, reaping children */
    if (pid == shell_pid && (W_WIFEXITED(status) || W_WIFSIGNALED(status))) {
      while (s_waitpid(-1, &status, false) > 0) {
      }
      break;
    }
  }
  return NULL;
}
