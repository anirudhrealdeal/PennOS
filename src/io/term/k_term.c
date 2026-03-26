/**
 * @file k_term.c
 * @brief Implementation of kernel-level terminal management.
 *
 * This file implements terminal ownership tracking for PennOS job control.
 * It maintains state about which process currently owns the terminal
 * (foreground process) and supports terminal inheritance during fork/exec.
 */

#include "k_term.h"
#include "io/filetable/k_filetable.h"
#include "io/fstd.h"
#include "process/schedule/k_schedule.h"

pid_t terminal_owner = -1, terminal_pass_parent = -1;

/**
 * @brief Check if a file descriptor refers to the terminal.
 *
 * A file descriptor refers to the terminal if its underlying host
 * file descriptor is STDIN_FILENO.
 *
 * @pre Interrupts are disabled
 * @param fd The file descriptor to check.
 * @return true if fd refers to the terminal, false otherwise.
 */
bool k_is_term(int fd) {
  ASSERT_NO_INTERRUPT("k_is_term");
  return k_fd_get_hfd(fd) == STDIN_FILENO;
}

/**
 * @brief Set the terminal owner to the specified process.
 *
 * @param pid The process ID to grant terminal ownership.
 */
void k_claim_terminal(pid_t pid) {
  terminal_owner = pid;
}

/**
 * @brief Get the parent process that should pass terminal to its child.
 *
 * @return The parent PID, or -1 if none is set.
 */
pid_t k_get_terminal_pass_parent() {
  return terminal_pass_parent;
}

/**
 * @brief Mark a parent to pass terminal ownership to its next child.
 *
 * @param parent The parent process ID that will pass terminal ownership.
 */
void k_pass_terminal_to_next_child(pid_t parent) {
  terminal_pass_parent = parent;
}

/**
 * @brief Get the current terminal owner.
 *
 * @return The terminal owner's PID, or -1 if no owner is set.
 */
pid_t k_get_terminal_owner() {
  return terminal_owner;
}