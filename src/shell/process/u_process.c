/**
 * @file process.c
 * @brief Implementation of user-level process management commands.
 *
 * This file implements the built-in shell commands for process management:
 * ps, kill, nice, nice_pid, logout, and sleep. These commands use the
 * s_* system call interface to interact with the PennOS kernel.
 */

#include "u_process.h"

#include <string.h>
#include "io/term/s_term.h"
#include "process/pcb/s_pcb.h"
#include "process/schedule/s_schedule.h"
#include "process/signal/s_signal.h"
#include "shell/commands.h"

/**
 * @brief Spawn a command with a specified priority.
 *
 * Spawns a new process with specified priority level (0=high, 1=med, 2=low).
 *
 * @param job_table Pointer to the shell's job table for background jobs.
 * @param argv NULL-terminated argument array.
 * @param cmd Parsed command structure for redirection/background handling.
 */
void u_nice(job_table_t* shell, char** argv, struct parsed_command* cmd) {
  /* Validate we have both priority and command arguments */
  if (argv[1] == NULL || argv[2] == NULL) {
    s_perror("nice: usage: nice <priority> <command> [args...]\n");
    s_perror("  priority: 0 (high), 1 (medium), 2 (low)\n");
    free(cmd);
    return;
  }
  /* Parse and validate priority is in valid range for scheduler */
  int priority = atoi(argv[1]);
  if (priority < 0 || priority > 2) {
    s_perror("nice: invalid priority %d (must be 0-2)\n", priority);
    free(cmd);
    return;
  }
  /* Pass priority to run_command which will apply it after spawning.
   * Skip first two args (nice and priority) to get actual command */
  run_command(argv + 2, cmd, priority, shell);
}

/**
 * @brief Change the priority of an existing process.
 *
 * Changes priority of already-running process by PID.
 *
 * @param argv NULL-terminated argument array.
 */
void u_nice_pid(char** argv) {
  if (argv[1] == NULL || argv[2] == NULL) {
    s_perror("nice_pid: usage: nice_pid <priority> <pid>\n");
    return;
  }

  int priority = atoi(argv[1]);
  pid_t pid = atoi(argv[2]);

  if (s_nice(pid, priority) == -1) {
    s_perror("nice_pid: failed to set priority\n");
  }
}

/**
 * @brief List all processes in the system.
 *
 * Displays all active processes with their PID, parent PID, priority,
 * state, and name in a formatted table.
 *
 * @param arg Unused parameter (for spthread compatibility).
 * @return Always NULL.
 */
void* u_ps(void* arg) {
  (void)arg;

  /* Print header */
  s_fprintf(STDOUT_FILENO, "PID PPID PRI STAT NAME\n");

/* Get process information via system call */
#define MAX_PS_PROCESSES 256
  process_info_t processes[MAX_PS_PROCESSES];
  int count = s_ps(processes, MAX_PS_PROCESSES);

  for (int i = 0; i < count; i++) {
    /* Determine state string */
    const char* state_str;
    switch (processes[i].state) {
      case 0: /* ACTIVE */
        state_str = "RUN";
        break;
      case 1: /* BLOCKED */
        state_str = "BLK";
        break;
      case 2: /* STOPPED */
        state_str = "STP";
        break;
      case 3: /* ZOMBIE */
        state_str = "ZMB";
        break;
      default:
        state_str = "???";
        break;
    }

    /* Print process information */
    s_fprintf(STDOUT_FILENO, "%-3d %-4d %-3d %-4s %s\n", processes[i].pid,
              (processes[i].par_pid == -1) ? 0 : processes[i].par_pid,
              processes[i].priority, state_str, processes[i].name);
  }

  return NULL;
}

/**
 * @brief Send signals to processes.
 *
 * Sends signals (SIGTERM/SIGSTOP/SIGCONT) to one or more processes
 * specified by their PIDs.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_kill(void* arg) {
  char** argv = (char**)arg;

  /* Check for correct number of arguments */
  if (argv[1] == NULL) {
    s_perror("kill: missing operand\n");
    s_perror("Usage: kill <pid> ...\n");
    return NULL;
  }

  int def_signal = SIGTERM;
  int idx = 1;

  if (argv[1][0] == '-') {
    if (strcmp(argv[1], "-term") == 0) {
      def_signal = SIGTERM;
      idx = 2;
    } else if (strcmp(argv[1], "-stop") == 0) {
      def_signal = SIGSTOP;
      idx = 2;
    } else if (strcmp(argv[1], "-cont") == 0) {
      def_signal = SIGCONT;
      idx = 2;
    } else {
      s_write(STDERR_FILENO, "kill: invalid signal\n", 18);
      return NULL;
    }
  }

  if (argv[idx] == NULL) {
    s_write(STDERR_FILENO, "invalid PID\n", 12);
    return NULL;
  }

  /* Send signal to each PID specified. Continue processing remaining PIDs
   * even if one fails */
  for (int i = idx; argv[i] != NULL; i++) {
    pid_t pid = (pid_t)atoi(argv[i]);
    if (pid <= 0) {
      s_perror("kill: invalid PID: %s\n", argv[i]);
      continue;
    }
    if (s_kill(pid, def_signal) < 0) {
      s_perror("kill: cannot signal process %d\n", pid);
      continue;
    }
  }
  return NULL;
}

/**
 * @brief Logout and shutdown PennOS.
 *
 * Initiates system shutdown by killing all processes and exiting cleanly.
 */
void u_logout(void) {
  /* Kill all processes and exit cleanly */
  s_logout();
  exit(1);
}

/**
 * @brief Sleep for a specified duration.
 *
 * Blocks the calling process for the specified number of seconds.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_sleep(void* arg) {
  char** argv = (char**)arg;
  if (argv == NULL || argv[0] == NULL || argv[1] == NULL) {
    s_perror("sleep: missing operand\n");
    return NULL;
  }

  unsigned int ticks = (unsigned int)atoi(argv[1]);
  s_sleep(ticks *
          10); /* Convert seconds to ticks (100ms each, so 10 per second) */

  return NULL;
}
