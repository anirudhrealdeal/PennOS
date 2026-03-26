/**
 * @file k_schedule.c
 * @brief Implementation of the PennOS priority scheduler.
 *
 * This file implements a multi-level priority scheduler with three queues
 * (high, medium, low priority). Processes are scheduled using weighted
 * round-robin with approximately 1.5x time ratio between priority levels.
 *
 * Key features:
 * - Three priority queues (pq0=high, pq1=medium, pq2=low)
 * - Weighted scheduling: pq0 gets 9 credits, pq1 gets 6, pq2 gets 4
 * - 100ms clock tick using SIGALRM
 * - Blocked queue for sleeping/waiting processes
 * - Interrupt disable counting for nested critical sections
 * - SIGINT/SIGTSTP handling for foreground process signaling
 */
#include "k_schedule.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "process/pcb/k_pcb.h"
#include "process/signal/s_signal.h"
#include "util/log.h"

// Priority queues - vectors holding PIDs for each priority level
static vector(pid_t) pq0 = NULL;  // High priority queue (priority 0)
static vector(pid_t) pq1 = NULL;  // Medium priority queue (priority 1)
static vector(pid_t) pq2 = NULL;  // Low priority queue (priority 2)

// Queue for blocked processes (sleeping or waiting)
static vector(pid_t) blocked_queue = NULL;

static pid_t current_pid = -1;
static bool scheduler_running = false;  // True while scheduler loop is active.

// Credit counters for weighted scheduling (1.5x ratio).
static int pq0_counter = 0;
static int pq1_counter = 0;
static int pq2_counter = 0;

// Round Robin indices for tracking position within each queue for fair
// scheduling.
static size_t pq0_index = 0;
static size_t pq1_index = 0;
static size_t pq2_index = 0;

// Counter for nested interrupt disable calls
static int interrupt_disable_counter = 0;

// Queue of PIDs pending SIGINT delivery
static vector(pid_t) to_int = NULL;

// Queue of PIDs pending SIGSTOP delivery.
static vector(pid_t) to_stop = NULL;

/**
 * @brief Signal handler for SIGALRM (clock tick).
 * @param signum Signal number (unused).
 */
static void alarm_handler(int signum) {
  (void)signum;
}

/**
 * @brief Signal handler for SIGINT (Ctrl-C).
 * @param sig Signal number (unused).
 */
static void sigint_handler(int sig) {
  if (to_int == NULL) {
    to_int = vector_new(pid_t, 3, NULL);
  }
  pid_t pid = s_get_terminal_owner();
  // pid 1 is init and 2 is shell
  if (pid > 2) {
    vector_push(&to_int, pid);
  }
}

/**
 * @brief Signal handler for SIGTSTP (Ctrl-Z).
 * Queues the terminal owner for SIGSTOP delivery during next scheduler tick.
 * @param sig Signal number (unused).
 */
static void sigstop_handler(int sig) {
  if (to_stop == NULL) {
    to_stop = vector_new(pid_t, 3, NULL);
  }
  pid_t pid = s_get_terminal_owner();
  if (pid > 2) {
    vector_push(&to_stop, pid);
  }
}

/**
 * @brief Initialize the scheduler.
 *
 * Allocates all queue vectors, sets up signal handlers for SIGALRM,
 * SIGINT, and SIGTSTP, and unblocks these signals.
 *
 * @return 0 on success, -1 if allocation fails.
 */
int k_sched_init(void) {
  pq0 = vector_new(pid_t, 16, NULL);
  pq1 = vector_new(pid_t, 16, NULL);
  pq2 = vector_new(pid_t, 16, NULL);
  blocked_queue = vector_new(pid_t, 16, NULL);
  to_int = vector_new(pid_t, 3, NULL);
  to_stop = vector_new(pid_t, 3, NULL);

  if (pq0 == NULL || pq1 == NULL || pq2 == NULL || blocked_queue == NULL) {
    return -1;
  }

  // Unblock sigint for shell thread
  sigset_t unblock_int;
  sigemptyset(&unblock_int);
  pthread_sigmask(SIG_UNBLOCK, &unblock_int, NULL);

  /* Set up SIGALRM handler */
  sigset_t suspend_set;
  sigfillset(&suspend_set);
  sigdelset(&suspend_set, SIGALRM);

  struct sigaction act_alarm = {
      .sa_handler = alarm_handler,
      .sa_mask = suspend_set,
      .sa_flags = SA_RESTART,
  };
  sigaction(SIGALRM, &act_alarm, NULL);

  sigset_t full_set;
  sigfillset(&full_set);
  // SIGINT handler for CTRL-C
  struct sigaction act_sigint = {
      .sa_handler = sigint_handler,
      .sa_mask = full_set,
      .sa_flags = SA_RESTART,
  };
  sigaction(SIGINT, &act_sigint, NULL);

  // SIGTSTP handler for CTRL-Z
  struct sigaction act_sigstp = {
      .sa_handler = sigstop_handler,
      .sa_mask = full_set,
      .sa_flags = SA_RESTART,
  };
  sigaction(SIGTSTP, &act_sigstp, NULL);

  /* Unblock all signals */
  sigset_t unblock_set;
  sigemptyset(&unblock_set);
  sigaddset(&unblock_set, SIGALRM);
  sigaddset(&unblock_set, SIGINT);
  sigaddset(&unblock_set, SIGTSTP);
  pthread_sigmask(SIG_UNBLOCK, &unblock_set, NULL);

  return 0;
}

/**
 * @brief Add a process to the appropriate queue.
 *
 * Routes to blocked_queue if state is BLOCKED, otherwise to
 * priority queue based on sched_priority (0, 1, or 2).
 *
 * @param pid Process ID to add.
 * @return 0 on success.
 */
int k_sched_add(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_sched_add");
  if (pq0 == NULL) {
    k_sched_init();
  }

  pcb_t* pcb = k_pcb_data(pid);

  // Add to blocked queue if blocked
  if (pcb->state == BLOCKED) {
    vector_push(&blocked_queue, pid);
    return 0;
  }

  /* Add to appropriate priority queue based on process priority */
  switch (pcb->sched_priority) {
    case 0:
      vector_push(&pq0, pid);
      break;
    case 1:
      vector_push(&pq1, pid);
      break;
    case 2:
      vector_push(&pq2, pid);
      break;
    default:
      vector_push(&pq1, pid); /* Default to priority 1 */
      break;
  }

  return 0;
}

/**
 * @brief Remove a PID from a specific queue.
 *
 * Helper function that searches the queue and removes the first match.
 *
 * @param queue Pointer to the queue vector.
 * @param pid Process ID to remove.
 */
static void remove_from_queue(vector(pid_t) * queue, pid_t pid) {
  for (int i = ((int)vector_len(queue)) - 1; i >= 0; i--) {
    if ((*queue)[i] == pid) {
      vector_erase(queue, i);
      return;
    }
  }
}

/**
 * @brief Remove a PID from all scheduler queues.
 *
 * Removes from pq0, pq1, pq2, and blocked_queue.
 *
 * @param pid Process ID to remove.
 */
void k_sched_remove(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_sched_remove");
  remove_from_queue(&pq0, pid);
  remove_from_queue(&pq1, pid);
  remove_from_queue(&pq2, pid);
  remove_from_queue(&blocked_queue, pid);
}

/**
 * @brief Remove PID from pending SIGINT queue.
 *
 * @param pid Process ID to remove.
 */
void k_remove_int(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_remove_int");
  remove_from_queue(&to_int, pid);
}

/**
 * @brief Remove PID from pending SIGSTOP queue.
 *
 * @param pid Process ID to remove.
 */
void k_remove_stop(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_remove_stop");
  remove_from_queue(&to_stop, pid);
}

/**
 * @brief Block a process.
 *
 * Removes from active queues, sets state to BLOCKED, adds to blocked_queue.
 *
 * @param pid Process ID to block.
 */
void k_sched_block(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_sched_block");
  k_sched_remove(pid);
  pcb_t* pcb = k_pcb_data(pid);
  pcb->state = BLOCKED;
  vector_push(&blocked_queue, pid);
  log_blocked(pid, pcb->sched_priority, pcb->process_name);
}

/**
 * @brief Unblock a process.
 *
 * Removes from blocked_queue, sets state to ACTIVE, re-adds to priority queue.
 *
 * @param pid Process ID to unblock.
 */
void k_sched_unblock(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_sched_unblock");
  remove_from_queue(&blocked_queue, pid);
  pcb_t* pcb = k_pcb_data(pid);
  pcb->state = ACTIVE;
  log_unblocked(pid, pcb->sched_priority, pcb->process_name);
  k_sched_add(pid);
}

/**
 * @brief Check blocked queue for processes ready to wake.
 *
 * Iterates through blocked_queue, unblocking any process whose
 * wakeup_time has been reached based on current tick count.
 */
static void check_blocked_queue(void) {
  unsigned int current_tick = log_get_ticks();

  for (size_t i = 0; i < vector_len(&blocked_queue);) {
    pid_t pid = blocked_queue[i];
    pcb_t* pcb = k_pcb_data(pid);

    /* Check if sleep time has elapsed */
    if (pcb->wakeup_time > 0 && current_tick >= pcb->wakeup_time) {
      pcb->wakeup_time = 0;
      k_sched_unblock(pid);
      /* Don't increment i since we removed an element */
    } else {
      i++;
    }
  }
}

/**
 * @brief Select the next process to run.
 *
 * Implements weighted priority scheduling:
 * 1. Check for waking blocked processes
 * 2. If only one priority level has processes, use simple round-robin
 * 3. Otherwise, use credit-based selection (9:6:4 ratio for pq0:pq1:pq2)
 *
 * @return PID of next process to run, or -1 if none available.
 */
static pid_t select_next_process(void) {
  check_blocked_queue();

  /* Check if all queues are empty */
  int has_pq0 = vector_len(&pq0) > 0;
  int has_pq1 = vector_len(&pq1) > 0;
  int has_pq2 = vector_len(&pq2) > 0;

  if (!has_pq0 && !has_pq1 && !has_pq2) {
    return -1; /* No runnable process */
  }

  /* If only one priority level has processes, just run from it */
  if (has_pq0 && !has_pq1 && !has_pq2) {
    pq0_index = pq0_index % vector_len(&pq0);
    return pq0[pq0_index++];
  }
  if (!has_pq0 && has_pq1 && !has_pq2) {
    pq1_index = pq1_index % vector_len(&pq1);
    return pq1[pq1_index++];
  }
  if (!has_pq0 && !has_pq1 && has_pq2) {
    pq2_index = pq2_index % vector_len(&pq2);
    return pq2[pq2_index++];
  }

  /* Multiple priority levels have processes - use weighted scheduling
   * Cycle of 19 quanta: pq0 gets 9, pq1 gets 6, pq2 gets 4
   * Pattern: 0,0,0,1,0,0,1,0,2,0,0,1,0,0,1,0,2,0,0 (roughly)
   * Simpler: use counters that track "debt" to each queue
   */

  int tot_sub = 0;
  /* Increment counters based on weights (credits per tick) */
  if (has_pq0) {
    pq0_counter += 9;
    tot_sub += 9;
  }
  if (has_pq1) {
    pq1_counter += 6;
    tot_sub += 6;
  }
  if (has_pq2) {
    pq2_counter += 4;
    tot_sub += 4;
  }

  /* Select from highest priority queue with most credits */
  pid_t selected = -1;

  if (has_pq0 && pq0_counter >= pq1_counter && pq0_counter >= pq2_counter) {
    pq0_index = pq0_index % vector_len(&pq0);
    selected = pq0[pq0_index++];
    pq0_counter -= tot_sub; /* Subtract full cycle cost */
  } else if (has_pq1 && pq1_counter >= pq2_counter) {
    pq1_index = pq1_index % vector_len(&pq1);
    selected = pq1[pq1_index++];
    pq1_counter -= tot_sub;
  } else if (has_pq2) {
    pq2_index = pq2_index % vector_len(&pq2);
    selected = pq2[pq2_index++];
    pq2_counter -= tot_sub;
  }

  return selected;
}

/**
 * @brief Main scheduler loop.
 *
 */
void k_sched_run(void) {
  scheduler_running = true;

  /* Set up timer for 100ms (clock tick) */
  struct itimerval it;
  it.it_interval = (struct timeval){.tv_sec = 0, .tv_usec = 100000}; /* 100ms */
  it.it_value = it.it_interval;
  setitimer(ITIMER_REAL, &it, NULL);

  /* Suspend set for sigsuspend */
  sigset_t suspend_set;
  sigfillset(&suspend_set);
  sigdelset(&suspend_set, SIGALRM);
  while (scheduler_running) {
    /* Deliver terminal signals */
    while (vector_len(&to_stop) > 0) {
      pid_t pid = vector_get(&to_stop, vector_len(&to_stop) - 1);
      s_kill(pid, SIGSTOP);
    }
    while (vector_len(&to_int) > 0) {
      pid_t pid = vector_get(&to_int, vector_len(&to_int) - 1);
      s_kill(pid, SIGINT);
    }

    /* Increment tick counter */
    log_tick();

    /* Select next process */
    pid_t next_pid = select_next_process();

    if (next_pid == -1) {
      if (vector_len(&blocked_queue) == 0) {
        /* No processes remain, exit */
        current_pid = -1;
        return;
      }
      /* All processes blocked - idle */
      sigsuspend(&suspend_set);
      continue;
    }

    /* Get PCB for next process */
    pcb_t* pcb = k_pcb_data(next_pid);

    /* Skip if process is not in ACTIVE state */
    if (pcb->state != ACTIVE) {
      continue;
    }

    /* Log schedule event */
    log_schedule(next_pid, pcb->sched_priority, pcb->process_name, false);

    /* Continue next process */
    current_pid = next_pid;
    spthread_continue(pcb->thread);

    sigsuspend(&suspend_set);
    while (interrupt_disable_counter != 0 &&
           k_pcb_data(next_pid)->state == ACTIVE) {
      log_tick();
      current_pid = next_pid;
      log_schedule(next_pid, pcb->sched_priority, pcb->process_name, true);
      /* Wait for next alarm */
      sigsuspend(&suspend_set);
    }

    /* Suspend the current process after quantum */
    interrupt_disable_counter = 1;
    pcb_t* curr_pcb = k_pcb_data(next_pid);
    if (curr_pcb->state == ACTIVE) {
      spthread_suspend(curr_pcb->thread);
    }
    interrupt_disable_counter = 0;
    current_pid = -1;
  }

  /* Stop timer */
  it.it_value = (struct timeval){0};
  setitimer(ITIMER_REAL, &it, NULL);
}

/**
 * @brief Stop the scheduler loop.
 */
void k_sched_stop(void) {
  scheduler_running = false;
}

/**
 * @brief Get current process PID.
 * @return Current PID or -1.
 */
pid_t k_sched_current_pid(void) {
  return current_pid;
}

/**
 * @brief Free all scheduler queue resources.
 * @return 0 on success.
 */
int k_sched_clean(void) {
  /* Free vectors if needed */
  if (pq0 != NULL) {
    vector_free(&pq0);
    pq0 = NULL;
  }
  if (pq1 != NULL) {
    vector_free(&pq1);
    pq1 = NULL;
  }
  if (pq2 != NULL) {
    vector_free(&pq2);
    pq2 = NULL;
  }
  if (blocked_queue != NULL) {
    vector_free(&blocked_queue);
    blocked_queue = NULL;
  }
  if (to_int != NULL) {
    vector_free(&to_int);
    to_int = NULL;
  }
  if (to_stop != NULL) {
    vector_free(&to_stop);
    to_stop = NULL;
  }
  return 0;
}

/**
 * @brief Disable interrupts (nested).
 * Increments counter; disables spthread interrupts on 0->1 transition.
 */
void k_disable_interrupts() {
  if (interrupt_disable_counter == 0) {
    spthread_disable_interrupts_self();
  }
  interrupt_disable_counter++;
}

/**
 * @brief Enable interrupts (nested).
 * Decrements counter; enables spthread interrupts on 1->0 transition.
 */
void k_enable_interrupts() {
  if (interrupt_disable_counter <= 0) {
    return;
  }
  interrupt_disable_counter--;
  if (interrupt_disable_counter == 0) {
    spthread_enable_interrupts_self();
  }
}

/**
 * @brief Check if interrupts are enabled.
 * @return true if process running and counter is 0.
 */
bool k_can_interrupt() {
  return current_pid != -1 && interrupt_disable_counter == 0;
}