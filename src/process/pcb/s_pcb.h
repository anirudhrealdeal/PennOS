/**
 * @file s_pcb.h
 * @brief System-level PCB information interface.
 *
 * This header provides the system call interface for querying and
 * modifying process information, including process listing (ps)
 * and priority adjustment (nice).
 */

#ifndef S_PCB_H_
#define S_PCB_H_

#include "process/pcb/pcb_state_t.h"
#include "process/pstd.h"

/** @brief Maximum process name length for s_ps output. */
#define MAX_PROCESS_NAME_PS 256

/**
 * @brief Process information structure for s_ps.
 *
 * Contains a snapshot of process information suitable for
 * display by the ps shell command.
 */
typedef struct {
  pid_t pid;     /**< Process ID. */
  pid_t par_pid; /**< Parent process ID. */
  int priority;  /**< Scheduling priority (0-2). */
  int state; /**< State: 0=ACTIVE, 1=BLOCKED, 2=STOPPED, 3=ZOMBIE, 4=WAITED. */
  char name[MAX_PROCESS_NAME_PS]; /**< Process name. */
} process_info_t;

/**
 * @brief Set the priority of the specified thread.
 *
 * @param pid Process ID of the target thread.
 * @param priority The new priorty value of the thread (0, 1, or 2)
 * @return 0 on success, -1 on failure.
 */
int s_nice(pid_t pid, int priority);

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
int s_ps(process_info_t* info, int max_processes);

/**
 * @brief Get the current state of a process.
 *
 * @param pid Process ID to query.
 * @return The process state (ACTIVE, BLOCKED, STOPPED, ZOMBIE, or WAITED).
 */
pcb_state_t s_proc_state(pid_t pid);

#endif  // S_PCB_H_