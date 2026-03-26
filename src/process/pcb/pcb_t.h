/**
 * @file pcb_t.h
 * @brief Process Control Block (PCB) structure definition.
 *
 * This header defines the PCB structure that contains all per-process
 * state information in PennOS, including thread handle, parent/child
 * relationships, file descriptors, scheduling info, and signal state.
 */

#ifndef PCB_H_
#define PCB_H_

#include "pcb_state_t.h"
#include "util/spthread.h"
#include "util/vector.h"

/** @brief Default scheduling priority for new processes. */
#define DEFAULT_SCHEDULE_PRIORITY 1

/** @brief Maximum length of process name string. */
#define MAX_PROCESS_NAME 256

/**
 * @brief Process Control Block structure.
 *
 * Contains all state information for a single process in PennOS.
 * Parent-child and sibling relationships are maintained as circular
 * doubly-linked lists for efficient traversal.
 */
typedef struct {
  spthread_t thread;  /**< Underlying spthread handle. */
  pid_t parent;       /**< Parent process PID (-1 if none). */
  pid_t alive_child;  /**< Head of alive children list (-1 if empty). */
  pid_t zombie_child; /**< Head of zombie children list (-1 if empty). */
  pid_t prev_sibling; /**< Previous sibling in circular list. */
  pid_t nex_sibling;  /**< Next sibling in circular list. */
  int sched_priority; /**< Scheduling priority (0=high, 1=mid, 2=low). */

  /**
   * @brief Per-process file descriptor table.
   *
   * Each entry is an index into the kernel-global fd table.
   * Index 0/1/2 typically map to stdin/stdout/stderr.
   */
  vector(int) fd_table;

  pcb_state_t state; /**< Current process state. */

  char process_name[MAX_PROCESS_NAME]; /**< Process name for display/logging. */

  /** @name Blocked process state
   *  @brief Fields used when process is in BLOCKED state.
   *  @{
   */
  unsigned int wakeup_time;  /**< Tick count to wake up (for s_sleep). */
  bool terminated_by_signal; /**< True if terminated by signal vs exit. */
  pid_t waitpid_target;      /**< Target PID for waitpid (-1 = any). */
  int* wstatus;              /**< Pointer to store wait status. */
  /** @} */

  /** @name Signal handling flags
   *  @brief Flags controlling signal behavior.
   *  @{
   */
  bool ign_int;  /**< Ignore S_SIGINT if true. */
  bool ign_stop; /**< Ignore S_SIGSTOP if true. */
  /** @} */

  // If transferred from BLOCKED to STOPPED, the time it happened, else 0.
  bool stopped_as_blocked;
} pcb_t;

#endif  // PCB_H_