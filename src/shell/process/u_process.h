/**
 * @file u_process.h
 * @brief User-level process management commands for the PennOS shell.
 *
 * This header declares built-in shell commands for process management,
 * including listing processes, sending signals, changing priorities,
 * sleeping, and system logout.
 */

#ifndef U_PROCESS_H_
#define U_PROCESS_H_

#include "shell/job/job_table_t.h"
#include "util/parser.h"

/**
 * @brief Spawn a command with a specified priority.
 *
 * Spawns a new process with specified priority level (0=high, 1=med, 2=low).
 *
 * @param job_table Pointer to the shell's job table for background jobs.
 * @param argv NULL-terminated argument array.
 * @param cmd Parsed command structure for redirection/background handling.
 */
void u_nice(job_table_t* shell, char** argv, struct parsed_command* cmd);

/**
 * @brief Change the priority of an existing process.
 *
 * Changes priority of already-running process by PID.
 *
 * @param argv NULL-terminated argument array.
 */
void u_nice_pid(char** argv);

/**
 * @brief List all processes in the system.
 *
 * Displays all active processes with their PID, parent PID, priority,
 * state, and name in a formatted table.
 *
 * @param arg Unused parameter (for spthread compatibility).
 * @return Always NULL.
 */
void* u_ps(void* arg);

/**
 * @brief Send signals to processes.
 *
 * Sends signals (SIGTERM/SIGSTOP/SIGCONT) to one or more processes
 * specified by their PIDs.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_kill(void* arg);

/**
 * @brief Logout and shutdown PennOS.
 *
 * Initiates system shutdown by killing all processes and exiting cleanly.
 */
void u_logout(void);

/**
 * @brief Sleep for a specified duration.
 *
 * Blocks the calling process for the specified number of seconds.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_sleep(void* arg);

#endif  // U_PROCESS_H_