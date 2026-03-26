/**
 * @file k_signal.h
 * @brief Kernel-level signal handling interface for PennOS.
 *
 * This header provides kernel-level functions for signal-related operations,
 * including polling for zombie children and notifying parent processes when
 * a child's state changes.
 */

#ifndef K_SIGNAL_H_
#define K_SIGNAL_H_

#include "process/pstd.h"


/**
 * @brief Poll for a child process that has changed state.
 *
 * Checks if the specified target child (or any child if target is -1)
 * has become a zombie. If found, reaps the child and returns its PID.
 *
 * @param parent PID of the parent process doing the poll.
 * @param target PID of specific child to poll, or -1 for any child.
 *               A value of 0 is treated as -1.
 * @param wstatus Pointer to store the exit status (W_EXITED or W_SIGNALED).
 *                Can be NULL if status is not needed.
 * @return Child PID if a zombie was reaped, 0 if no zombie found,
 *         -1 on error (invalid PID or not a child of parent).
 */
pid_t k_pollpid(pid_t parent, pid_t target, int* wstatus);


/**
 * @brief Notify a parent process of a child's state change.
 *
 * Called when a child process changes state (zombified, stopped, continued).
 * If the parent is blocked waiting for this child (or any child), unblocks
 * the parent and sets the waitpid return values.
 *
 * @param child PID of the child that changed state.
 * @param wstatus Status value to report (W_EXITED, W_SIGNALED, W_STOPPED, W_CONTINUED).
 */
void k_notify_parent(pid_t child, int wstatus);

#endif  // K_SIGNAL_H_