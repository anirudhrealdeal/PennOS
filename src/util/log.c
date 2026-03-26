/**
 * @file log.c
 * @brief Implementation of the PennOS scheduler event logger.
 *
 * This file implements logging of scheduler events to a file for debugging
 * and grading purposes. Each log entry includes a timestamp (clock ticks),
 * event type, process ID, priority, and process name.
 *
 * Log format: [ticks]\tEVENT\tpid\tpriority\tprocess_name
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief File handle for log output. */
static FILE* log_file_handle = NULL;

/** @brief Global clock tick counter (100ms per tick). */
static unsigned int clock_ticks = 0;

/**
 * @brief Initialize the logging system.
 *
 * Opens the log file for writing, closes any previously open file,
 * and resets the clock tick counter to 0.
 *
 * @param log_file Path to the log file.
 * @return 0 on success, -1 on error.
 */
int log_init(const char* log_file) {
  if (log_file_handle != NULL) {
    fclose(log_file_handle);
  }

  log_file_handle = fopen(log_file, "w");
  if (log_file_handle == NULL) {
    perror("log_init: fopen");
    return -1;
  }

  clock_ticks = 0;
  return 0;
}

/**
 * @brief Close the logging system.
 *
 * Closes the log file if open and sets handle to NULL.
 */
void log_close(void) {
  if (log_file_handle != NULL) {
    fclose(log_file_handle);
    log_file_handle = NULL;
  }
}

/**
 * @brief Get the current clock tick count.
 *
 * @return Number of clock ticks since log_init.
 */
unsigned int log_get_ticks(void) {
  return clock_ticks;
}

/**
 * @brief Increment the clock tick counter.
 *
 * Called by the scheduler on each SIGALRM (100ms interval).
 */
void log_tick(void) {
  clock_ticks++;
}

/**
 * @brief Log a process scheduling event.
 *
 * Logs SCHEDULE or RESCHEDULE (if interrupts were disabled).
 *
 * @param pid Process ID being scheduled.
 * @param queue Priority queue (0, 1, or 2).
 * @param process_name Name of the process.
 * @param interrupt True if process had interrupts disabled.
 */
void log_schedule(pid_t pid,
                  int queue,
                  const char* process_name,
                  bool interrupt) {
  if (log_file_handle == NULL) {
    return;
  }
  if (interrupt) {
    fprintf(log_file_handle,
            "[%u]\tRESCHEDULE\t%d\t%d\t%s (disabled interrupts)\n", clock_ticks,
            pid, queue, process_name);
  } else {
    fprintf(log_file_handle, "[%u]\tSCHEDULE\t%d\t%d\t%s\n", clock_ticks, pid,
            queue, process_name);
  }
  fflush(log_file_handle);
}

/**
 * @brief Log process creation.
 *
 * @param pid New process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_create(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tCREATE\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process termination by signal.
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_signaled(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tSIGNALED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log normal process exit.
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_exited(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tEXITED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process entering zombie state.
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_zombie(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tZOMBIE\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process being orphaned.
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_orphan(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tORPHAN\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process being reaped by wait.
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 * @param is_init True if reaped by init process.
 */
void log_waited(pid_t pid,
                int nice_value,
                const char* process_name,
                bool is_init) {
  if (log_file_handle == NULL) {
    return;
  }
  if (is_init) {
    fprintf(log_file_handle, "[%u]\tWAITED\t%d\t%d\t%s (by init)\n",
            clock_ticks, pid, nice_value, process_name);
    fflush(log_file_handle);
  } else {
    fprintf(log_file_handle, "[%u]\tWAITED\t%d\t%d\t%s\n", clock_ticks, pid,
            nice_value, process_name);
    fflush(log_file_handle);
  }
}

/**
 * @brief Log priority change.
 *
 * @param pid Process ID.
 * @param old_nice Previous priority.
 * @param new_nice New priority.
 * @param process_name Name of the process.
 */
void log_nice(pid_t pid, int old_nice, int new_nice, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tNICE\t%d\t%d\t%d\t%s\n", clock_ticks, pid,
          old_nice, new_nice, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process blocking (sleep/wait).
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_blocked(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tBLOCKED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process unblocking (wakeup).
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_unblocked(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tUNBLOCKED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process being stopped (SIGSTOP).
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_stopped(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tSTOPPED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}

/**
 * @brief Log process being continued (SIGCONT).
 *
 * @param pid Process ID.
 * @param nice_value Process priority.
 * @param process_name Name of the process.
 */
void log_continued(pid_t pid, int nice_value, const char* process_name) {
  if (log_file_handle == NULL) {
    return;
  }
  fprintf(log_file_handle, "[%u]\tCONTINUED\t%d\t%d\t%s\n", clock_ticks, pid,
          nice_value, process_name);
  fflush(log_file_handle);
}
