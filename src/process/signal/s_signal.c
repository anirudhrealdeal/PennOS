/**
 * @file s_signal.c
 * @brief Implementation of system-level signal functions.
 *
 * This file implements the user-facing signal system calls for PennOS,
 * including s_kill for sending signals, s_logout for system shutdown,
 * and functions to manage signal ignore masks for SIGINT and SIGSTOP.
 */

#include "s_signal.h"
#include "k_signal.h"

#include "process/lifecycle/k_lifecycle.h"
#include "process/lifecycle/s_lifecycle.h"
#include "process/pcb/k_pcb.h"
#include "process/pcb/pcb_t.h"
#include "process/pstd.h"
#include "process/schedule/k_schedule.h"
#include "util/log.h"

/**
 * @brief Send a signal to a process.
 * @param pid Target process ID.
 * @param signal Signal number (SIGSTOP, SIGCONT, SIGINT, SIGTERM).
 * @return 0 on success, -1 on error.
 */
int s_kill(pid_t pid, int signal) {
  /* Protect init (PID 1) and shell (PID 2) from being killed/stopped
   * to maintain system stability */
  if (pid <= 1)
    return -1;
  k_disable_interrupts();
  pcb_t* target = k_pcb_data(pid);
  if (target == NULL) {
    k_enable_interrupts();
    return -1;
  }

  switch (signal) {
    case SIGSTOP: {
      /* Only stop if process is runnable. ZOMBIE/STOPPED states ignore SIGSTOP
       */
      if (target->state == ACTIVE || target->state == BLOCKED) {
        if (!s_can_sig_stop(pid)) {
          k_remove_stop(pid);
          return 0;
        }

        /* Special handling for stopping blocked tasks, so they can resume
         * blocked */
        if (target->state == BLOCKED) {
          target->stopped_as_blocked = true;

          /* Processes who got stopped while sleeping should not work towards
           * the final sleep timer */
          if (target->wakeup_time > 0) {
            if (log_get_ticks() < target->wakeup_time) {
              target->wakeup_time -= log_get_ticks();
            } else {
              /* Process should wakeup anyways, we can treat it like ACTIVE */
              target->stopped_as_blocked = false;
              target->wakeup_time = 0;
            }
          }
        }

        target->state = STOPPED;
        /* remove from scheduler queues */
        k_sched_remove(pid);
        k_remove_stop(pid);
        k_notify_parent(pid, W_STOPPED);
        log_stopped(pid, target->sched_priority, target->process_name);
        /* suspend the thread so it stops executing */
        spthread_suspend(target->thread);
      }
      break;
    }
    case SIGCONT: {
      /* Only resume if actually stopped */
      if (target->state == STOPPED) {
        if (target->stopped_as_blocked) {
          /* Was blocked when stopped - restore to blocked state */
          if (target->wakeup_time) {
            /* Convert relative time back to absolute wakeup time */
            target->wakeup_time += log_get_ticks();
          }
          target->state = BLOCKED;
          target->stopped_as_blocked = false;
        } else {
          /* Was active when stopped - restore to active state */
          target->state = ACTIVE;
        }
        /* Add back to scheduler queues for execution */
        k_sched_add(pid);

        /* Notify parent for potential waitpid with WCONTINUED flag */
        k_notify_parent(pid, W_CONTINUED);
        log_continued(pid, target->sched_priority, target->process_name);
      }
      break;
    }
    case SIGINT:
    case SIGTERM: {
      /* Check SIGINT ignore flag (SIGTERM always terminates) */
      if (signal == SIGINT && !s_can_sig_int(pid)) {
        k_remove_int(pid);
        return 0;
      }

      /* Mark as signal-terminated so waitpid can distinguish from normal exit
       */
      target->terminated_by_signal = true;

      /* zombify will handle reparenting/logging/waking the parent */
      log_signaled(pid, target->sched_priority, target->process_name);
      k_proc_zombify(pid, true);
      /* Cancel thread to forcibly stop execution */
      spthread_cancel(target->thread);
      break;
    }
    default: {
      perror("s_kill: illegal signal received");
      k_enable_interrupts();
      return -1;
    }
  }
  k_enable_interrupts();
  return 0;
}

/**
 * @brief Perform system logout and shutdown.
 */
void s_logout(void) {
  pid_t current = k_sched_current_pid();

  k_disable_interrupts();
  /* First pass: kill all processes except current and init (PID 1) */
  size_t pid = 1;
  do {
    /* Skip current process and init (PID 1) */
    if ((pid_t)pid != current && pid != 1) {
      pcb_t* pcb = k_pcb_data((pid_t)pid);
      /* Only kill if not already a zombie */
      if (pcb != NULL && pcb->state != ZOMBIE) {
        s_kill((pid_t)pid, SIGTERM);
      }
    }
  } while ((pid = k_next_process(pid + 1)) != 0);
  k_enable_interrupts();

  /* Second pass: reap all zombie children */
  int status;
  while (s_waitpid(-1, &status, true) > 0) {
    /* Keep reaping until no more zombies */
  }

  /* Now exit the current process - init will detect this and shutdown */
  s_exit();
}

/**
 * @brief Check if a process accepts SIGSTOP.
 *
 * Thread-safe check of the process's ign_stop flag.
 *
 * @param pid Process ID to check.
 * @return true if SIGSTOP will be handled, false if ignored.
 */
bool s_can_sig_stop(pid_t pid) {
  k_disable_interrupts();
  bool res = !k_pcb_data(pid)->ign_stop;
  k_enable_interrupts();
  return res;
}

/**
 * @brief Check if a process accepts SIGINT.
 *
 * Thread-safe check of the process's ign_int flag.
 *
 * @param pid Process ID to check.
 * @return true if SIGINT will be handled, false if ignored.
 */
bool s_can_sig_int(pid_t pid) {
  k_disable_interrupts();
  bool res = !k_pcb_data(pid)->ign_int;
  k_enable_interrupts();
  return res;
}

/**
 * @brief Set SIGINT ignore flag for a process.
 *
 * Thread-safe modification of the process's ign_int flag.
 *
 * @param pid Process ID to modify.
 * @param ignore true to ignore SIGINT, false to handle.
 */
void s_ignore_int(pid_t pid, bool ignore) {
  k_disable_interrupts();
  k_pcb_data(pid)->ign_int = ignore;
  k_enable_interrupts();
}

/**
 * @brief Set SIGSTOP ignore flag for a process.
 *
 * Thread-safe modification of the process's ign_stop flag.
 *
 * @param pid Process ID to modify.
 * @param ignore true to ignore SIGSTOP, false to handle.
 */
void s_ignore_stop(pid_t pid, bool ignore) {
  k_disable_interrupts();
  k_pcb_data(pid)->ign_stop = ignore;
  k_enable_interrupts();
}
