/**
 * @file k_term.h
 * @brief Kernel-level terminal management interface.
 *
 * This header provides the kernel-level interface for terminal ownership
 * and control in PennOS. It manages which process currently owns the
 * terminal (foreground process) for job control purposes.
 */

#ifndef K_TERM_H_
#define K_TERM_H_

#include "process/pstd.h"

/**
 * @brief Check if a file descriptor refers to the terminal.
 *
 * Determines whether the given file descriptor is connected to
 * standard input (the terminal).
 *
 * @param fd The file descriptor to check.
 * @return true if fd refers to the terminal, false otherwise.
 */
bool k_is_term(int fd);

/**
 * @brief Set the terminal owner to the specified process.
 *
 * Grants terminal ownership to the given process, making it the
 * foreground process for terminal I/O.
 *
 * @param pid The process ID to grant terminal ownership.
 */
void k_claim_terminal(pid_t pid);

/**
 * @brief Get the parent process that should pass terminal to its child.
 *
 * Returns the PID of the parent process that has been marked to
 * pass terminal ownership to its next child process.
 *
 * @return The parent PID, or -1 if none is set.
 */
pid_t k_get_terminal_pass_parent();

/**
 * @brief Mark a parent to pass terminal ownership to its next child.
 *
 * Used during process creation to ensure the child process inherits
 * terminal ownership from its parent (for foreground job execution).
 *
 * @param parent The parent process ID that will pass terminal ownership.
 */
void k_pass_terminal_to_next_child(pid_t parent);

/**
 * @brief Get the current terminal owner.
 *
 * Returns the PID of the process that currently owns the terminal
 * (the foreground process).
 *
 * @return The terminal owner's PID, or -1 if no owner is set.
 */
pid_t k_get_terminal_owner();

#endif  // K_TERM_H_