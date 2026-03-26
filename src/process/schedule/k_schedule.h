/**
 * @file k_schedule.h
 * @brief Kernel-level scheduler interface for PennOS.
 *
 * This header provides the kernel-level interface for the PennOS priority
 * scheduler. It supports three priority levels with weighted round-robin
 * scheduling, process blocking/unblocking, and interrupt management for
 * critical sections.
 *
 * @note The scheduler uses SIGALRM for 100ms clock ticks and handles
 *       SIGINT/SIGTSTP for terminal foreground process signaling.
 */

#ifndef K_SCHEDULE_H_
#define K_SCHEDULE_H_

#include "io/term/s_term.h"
#include "process/pcb/pcb_t.h"

/**
 * @brief Perform necessary initializations for a scheduler.
 *
 * @return 0 on success, -1 on error.
 */
int k_sched_init(void);

/**
 * @brief Add a process to the scheduler. This can happen regardless of whether
 * the scheduler is running.
 *
 * @return 0 on success, -1 on error.
 */
int k_sched_add(pid_t pid);

/**
 * @brief Remove a process from all scheduler queues.
 *
 * @param pid Process ID to remove
 */
void k_sched_remove(pid_t pid);

void k_remove_int(pid_t pid);
void k_remove_stop(pid_t pid);

/**
 * @brief Block a process (remove from active queues, add to blocked queue).
 *
 * @param pid Process ID to block
 */
void k_sched_block(pid_t pid);

/**
 * @brief Unblock a process (remove from blocked queue, add back to active
 * queue).
 *
 * @param pid Process ID to unblock
 */
void k_sched_unblock(pid_t pid);

/**
 * @brief Start the scheduler.
 *
 * Begins the main scheduling loop with 100ms timer ticks. Implements weighted
 * priority scheduling (9:6:4 ratio) and handles signal delivery.
 */
void k_sched_run(void);

/**
 * @brief Stop the scheduler.
 */
void k_sched_stop(void);

/**
 * @brief Get the currently running process PID.
 *
 * @return Current PID, or -1 if none
 */
pid_t k_sched_current_pid(void);

/**
 * @brief Clean up the scheduler, for when the program is shutting down.
 *
 * @return 0 on success, -1 on error.
 */
int k_sched_clean(void);

/**
 * @brief Disable interrupts (nested).
 *
 * Increments counter; disables spthread interrupts on 0->1 transition.
 * Supports nested calls for critical sections.
 */
void k_disable_interrupts();

/**
 * @brief Enable interrupts (nested).
 *
 * Decrements counter; enables spthread interrupts on 1->0 transition.
 * Must match number of k_disable_interrupts() calls.
 */
void k_enable_interrupts();

/**
 * @brief Check if interrupts are enabled.
 *
 * Returns true if a process is running and interrupt counter is 0.
 *
 * @return true if interrupts enabled, false otherwise.
 */
bool k_can_interrupt();

/** @brief Enable debug mode for scheduler assertions. */
#ifdef DEBUG
#define ASSERT_NO_INTERRUPT(func_name)                              \
  ({                                                                \
    if (k_can_interrupt()) {                                        \
      s_perror("%s: Calling function did not disable interrupts\n", \
               func_name);                                          \
      abort();                                                      \
    }                                                               \
  })
#else
#define ASSERT_NO_INTERRUPT(func_name) (void)0
#endif

#endif  // K_SCHEDULE_H_