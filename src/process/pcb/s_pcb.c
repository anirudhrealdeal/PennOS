/**
 * @file s_pcb.c
 * @brief Implementation of system-level PCB operations.
 *
 * This file implements the system calls for process information
 * and priority management: s_nice, s_ps, and s_proc_state.
 */

#include "s_pcb.h"

#include "process/pcb/k_pcb.h"
#include "process/pstd.h"
#include "process/schedule/k_schedule.h"

#include <string.h>
#include "util/log.h"

/**
 * @brief Set the priority of the specified thread.
 *
 * @param pid Process ID of the target thread.
 * @param priority The new priorty value of the thread (0, 1, or 2)
 * @return 0 on success, -1 on failure.
 */
int s_nice(pid_t pid, int priority) {
  /* Disable interrupts to make remove-update-add sequence atomic. */
  k_disable_interrupts();

  /* Remove from current priority queue before changing priority */
  k_sched_remove(pid);
  k_pcb_data(pid)->sched_priority = priority;

  /* Add back to appropriate queue based on new priority */
  k_sched_add(pid);
  k_enable_interrupts();
  return 0;
}

/**
 * @brief Get information about all active processes.
 *
 * Fills the provided array with process information for all
 * non-WAITED processes in the system.
 *
 * @param[out] info Array to fill with process information.
 * @param max_processes Maximum number of entries to fill.
 * @return Number of processes written to info array.
 */
int s_ps(process_info_t* info, int max_processes) {
  /* Lock interrupts for entire iteration to get consistent snapshot. */
  k_disable_interrupts();

  /* Start at PID 1 to skip the bootstrap process at PID 0 */
  int count = 0;
  pid_t pid = 1;
  do {
    pcb_t* pcb = k_pcb_data((pid_t)pid);
    info[count].pid = (pid_t)pid;
    info[count].par_pid = pcb->parent;
    info[count].priority = pcb->sched_priority;
    info[count].state = (int)pcb->state;

    /* Copy name with explicit null termination to prevent buffer overruns
     * if process_name wasn't properly null-terminated */
    strncpy(info[count].name, pcb->process_name, MAX_PROCESS_NAME_PS - 1);
    info[count].name[MAX_PROCESS_NAME_PS - 1] = '\0';
    count++;
  } while ((pid = k_next_process(pid + 1)) != 0);
  k_enable_interrupts();
  return count;
}

/**
 * @brief Get the current state of a process.
 *
 * @param pid Process ID to query.
 * @return The process state (ACTIVE, BLOCKED, STOPPED, ZOMBIE, or WAITED).
 */
pcb_state_t s_proc_state(pid_t pid) {
  k_disable_interrupts();
  pcb_state_t state = k_pcb_data(pid)->state;
  k_enable_interrupts();
  return state;
}