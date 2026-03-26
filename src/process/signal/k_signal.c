/**
 * @file k_signal.c
 * @brief Implementation of kernel-level signal handling.
 *
 * This file implements the kernel-level signal infrastructure for PennOS,
 * including the mechanism for parent-child notification on state changes
 * and polling for zombie children during waitpid operations.
 */

#include "k_signal.h"

#include "process/lifecycle/k_lifecycle.h"
#include "process/pcb/k_pcb.h"
#include "process/schedule/k_schedule.h"

/**
 * @brief Notify a parent process of a child's state change.
 * @param child PID of the child that changed state.
 * @param wstatus Status value (W_EXITED, W_SIGNALED, W_STOPPED, W_CONTINUED).
 * @pre Interrupts are disabled
 */
void k_notify_parent(pid_t child, int wstatus) {
  ASSERT_NO_INTERRUPT("k_notify_parent");
  pid_t parent = k_pcb_data(child)->parent;
  if (parent > 0) {
    pcb_t* data = k_pcb_data(parent);

    /* Only wake parent if it's blocked in waitpid waiting for this child.
     * Check both BLOCKED (normal wait) and stopped_as_blocked (process
     * was stopped via Ctrl-Z while waiting) */
    if ((data->state == BLOCKED || data->stopped_as_blocked) &&
        (data->waitpid_target == -1 || data->waitpid_target == child)) {
      /* Store which child triggered the wakeup so waitpid knows which to return
       */
      data->waitpid_target = child;

      /* Store status if parent provided a wstatus buffer */
      if (data->wstatus) {
        *data->wstatus = wstatus;
      }
      if (data->stopped_as_blocked) {
        /* Parent is stopped (Ctrl-Z) - don't actually wake it, just clear flag
         * so when it's continued it remains blocked in waitpid */
        data->stopped_as_blocked = false;
      } else {
        /* Parent is blocked waiting - wake it up by marking active and
         * unblocking */
        data->state = ACTIVE;
        k_sched_unblock(parent);
      }
    }
  }
}

/**
 * @brief Poll for a zombie child process.
 * @param parent PID of the parent process.
 * @param target PID of specific child or -1 for any.
 * @param wstatus Pointer to store exit status, can be NULL.
 * @pre Interrupts are disabled
 * @return Child PID if reaped, 0 if no zombie, -1 on error.
 */
pid_t k_pollpid(pid_t parent, pid_t target, int* wstatus) {
  ASSERT_NO_INTERRUPT("k_pollpid");

  /* Normalize target=0 to target=-1 (both mean "any child") */
  if (target == 0) {
    target = -1;
  }
  pcb_t* data = k_pcb_data(parent);

  /* Use zombie_child hint when searching for any child to avoid scanning
   * entire child list. */
  int chld = (target != -1) ? target : data->zombie_child;
  if (chld != -1) {
    /* Validate PID is within bounds to prevent out-of-bounds access */
    if (chld < 0 || chld >= k_pcb_table_size()) {
      return -1;
    }
    pcb_t* chld_data = k_pcb_data(chld);

    /* Verify this is actually our child */
    if (chld_data->parent != parent) {
      return -1;
    }
    /* Only reap if child is zombie. Other states (ACTIVE, STOPPED, etc.)
     * return 0 to indicate no ready child */
    if (chld_data->state == ZOMBIE) {
      if (wstatus) {
        /* Distinguish between normal exit and killed by signal */
        *wstatus = (chld_data->terminated_by_signal) ? W_SIGNALED : W_EXITED;
      }
      /* Actually reap the zombie, freeing its PCB */
      k_proc_cleanup(chld);
      return chld;
    }
  }
  /* No zombie child found */
  return 0;
}
