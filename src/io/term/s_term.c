/**
 * @file s_term.c
 * @brief Implementation of system-level terminal operations.
 *
 * This file implements the system call wrappers for terminal ownership
 * management in PennOS. These functions provide safe access to kernel
 * terminal state by disabling interrupts during critical sections.
 */

#include "s_term.h"
#include "k_term.h"
#include "process/schedule/k_schedule.h"

/**
 * @brief Set the terminal owner to the specified process.
 *
 * Disables interrupts before calling the kernel function to ensure
 * atomic modification of terminal ownership state.
 *
 * @param pid The process ID to grant terminal ownership.
 */
void s_claim_terminal(pid_t pid) {
  k_disable_interrupts();
  k_claim_terminal(pid);
  k_enable_interrupts();
}

/**
 * @brief Get the current terminal owner.
 *
 * @return The terminal owner's PID, or -1 if no owner is set.
 */
pid_t s_get_terminal_owner() {
  return k_get_terminal_owner();
}

/**
 * @brief Mark a parent to pass terminal ownership to its next child.
 *
 * Disables interrupts before calling the kernel function to ensure
 * atomic modification of terminal pass state.
 *
 * @param parent The parent process ID that will pass terminal ownership.
 */
void s_pass_terminal_to_next_child(pid_t parent) {
  k_disable_interrupts();
  k_pass_terminal_to_next_child(parent);
  k_enable_interrupts();
}

/**
 * @brief Get the parent process that should pass terminal to its child.
 *
 * @return The parent PID, or -1 if none is set.
 */
pid_t s_get_terminal_pass_parent() {
  return k_get_terminal_pass_parent();
}
