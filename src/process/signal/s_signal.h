/**
 * @file s_signal.h
 * @brief System-level signal interface for user processes.
 *
 * This header provides the system call interface for signal operations
 * in PennOS, including sending signals to processes, system logout,
 * and signal ignore/unignore functionality.
 */

#ifndef S_SIGNAL_H_
#define S_SIGNAL_H_

#include "process/pstd.h"

/**
 * @brief Send a signal to a particular process.
 *
 * @param pid Process ID of the target proces.
 * @param signal Signal number to be sent.
 * @return 0 on success, -1 on error.
 */
int s_kill(pid_t pid, int signal);

/**
 * @brief Logout and shutdown PennOS.
 *
 * This function kills all processes except the current one (using SIGTERM),
 * waits for them to be reaped, then exits the current process.
 */
void s_logout(void);

/**
 * @brief Check if a process can receive SIGSTOP.
 *
 * @param pid Process ID to check.
 * @return true if the process will handle SIGSTOP, false if ignoring.
 */
bool s_can_sig_stop(pid_t pid);

/**
 * @brief Check if a process can receive SIGINT.
 *
 * @param pid Process ID to check.
 * @return true if the process will handle SIGINT, false if ignoring.
 */
bool s_can_sig_int(pid_t pid);

/**
 * @brief Set whether a process ignores SIGSTOP signals.
 *
 * Used by background processes to prevent Ctrl-Z from stopping them.
 *
 * @param pid Process ID to modify.
 * @param ignore true to ignore SIGSTOP, false to handle it.
 */
void s_ignore_stop(pid_t pid, bool ignore);

/**
 * @brief Set whether a process ignores SIGINT signals.
 *
 * Used by background processes to prevent Ctrl-C from interrupting them.
 *
 * @param pid Process ID to modify.
 * @param ignore true to ignore SIGINT, false to handle it.
 */
void s_ignore_int(pid_t pid, bool ignore);

#endif  // S_SIGNAL_H