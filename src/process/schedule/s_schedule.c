/**
 * @file s_schedule.c
 * @brief Implementation of system-level scheduling functions.
 *
 * This file implements the user-facing system calls for scheduling:
 * s_getpid for obtaining the current process ID and s_sleep for
 * suspending execution for a specified duration.
 */

#include "s_schedule.h"

#include "process/pcb/k_pcb.h"
#include "process/pcb/pcb_t.h"
#include "process/schedule/k_schedule.h"
#include "util/log.h"

/**
 * @brief Suspends execution of the calling proces for a specified number of
 * clock ticks.
 *
 * This function is analogous to `sleep(3)` in Linux, with the behavior that the
 * system clock continues to tick even if the call is interrupted.
 *
 * @param ticks Duration of the sleep in system clock ticks. Must be greater
 * than 0.
 */
void s_sleep(unsigned int ticks) {
  if (ticks == 0)
    return;

  pid_t cur = k_sched_current_pid();
  if (cur < 0)
    return;

  k_disable_interrupts();
  pcb_t* pcb = k_pcb_data(cur);

  /* Set wakeup time in ticks and block the process in the scheduler */
  pcb->wakeup_time = log_get_ticks() + ticks;
  k_sched_block(cur);
  k_enable_interrupts();

  /* Suspend the calling thread until scheduler unblocks it */
  spthread_suspend_self();
}

/**
 * @brief Get the current process ID.
 * @return The current process ID.
 */
pid_t s_getpid(void) {
  return k_sched_current_pid();
}
