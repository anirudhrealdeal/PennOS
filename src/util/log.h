/**
 * @file log.h
 * @brief Scheduler event logging interface for PennOS.
 *
 * This header provides functions for logging scheduler events to a file,
 * including process creation, scheduling, state changes, and termination.
 * The log format is: [ticks] EVENT pid priority process_name
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include "process/pcb/pcb_t.h"

/**
 * @brief Initialize the logging system
 *
 * @param log_file File path for logging output
 * @return 0 on success, -1 on error
 */
int log_init(const char* log_file);

/**
 * @brief Close the logging system
 */
void log_close(void);

/**
 * @brief Get current clock ticks since boot
 *
 * @return Number of clock ticks
 */
unsigned int log_get_ticks(void);

/**
 * @brief Increment the clock tick counter
 */
void log_tick(void);

/**
 * @brief Log a schedule event
 *
 * @param pid Process ID
 * @param queue Priority queue (0, 1, or 2)
 * @param process_name Name of the process
 */
void log_schedule(pid_t pid,
                  int queue,
                  const char* process_name,
                  bool interrupt);

/**
 * @brief Log process creation
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_create(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process signaled termination
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_signaled(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process normal exit
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_exited(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process zombification
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_zombie(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process orphaning
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_orphan(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process wait/reap
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_waited(pid_t pid,
                int nice_value,
                const char* process_name,
                bool is_init);

/**
 * @brief Log nice value change
 *
 * @param pid Process ID
 * @param old_nice Old priority value
 * @param new_nice New priority value
 * @param process_name Name of the process
 */
void log_nice(pid_t pid, int old_nice, int new_nice, const char* process_name);

/**
 * @brief Log process blocking
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_blocked(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process unblocking
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_unblocked(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process stop
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_stopped(pid_t pid, int nice_value, const char* process_name);

/**
 * @brief Log process continuation
 *
 * @param pid Process ID
 * @param nice_value Priority value
 * @param process_name Name of the process
 */
void log_continued(pid_t pid, int nice_value, const char* process_name);

#endif  // LOG_H_
