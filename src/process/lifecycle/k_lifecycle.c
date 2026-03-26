/**
 * @file k_lifecycle.c
 * @brief Implementation of kernel-level process lifecycle management.
 *
 * This file implements process creation, termination, and cleanup for PennOS.
 * It manages the parent-child relationships, orphan reparenting to init,
 * and proper cleanup of process resources upon termination.
 *
 * @note All functions that modify PCB state assert they are not called
 *       from interrupt context to prevent race conditions.
 */

#include "k_lifecycle.h"

#include "io/filetable/k_filetable.h"
#include "process/pcb/k_pcb.h"
#include "process/schedule/k_schedule.h"
#include "process/signal/k_signal.h"
#include "util/log.h"

#include <string.h>

/** @brief PID of the process that adopts orphaned children (init process). */
size_t ORPHAN_FALLBACK = 1;

/**
 * @brief Transfer a list of children to a new parent.
 * @param[in,out] head Pointer to the new parent's child list head.
 * @param to_move Head of the circular sibling list to transfer.
 * @param new_parent PID of the new parent process.
 * @pre Interrupts are disabled
 */
void transfer_parent(pid_t* head, pid_t to_move, pid_t new_parent) {
  ASSERT_NO_INTERRUPT("transfer_parent");
  if (to_move == -1)
    return;

  /* Walk circular sibling list. Each iteration updates one child's parent
   * pointer */
  pid_t start = to_move;
  do {
    pcb_t* data = k_pcb_data(to_move);
    data->parent = new_parent;

    /* Notify new parent of inherited zombies so they can be reaped.
     * Without this, waitpid(-1) in new parent wouldn't know about these zombies
     */
    if (data->state == ZOMBIE) {
      k_notify_parent(to_move,
                      data->terminated_by_signal ? W_SIGNALED : W_EXITED);
    }
    to_move = data->nex_sibling;
  } while (to_move != start);

  /* Insert entire list at once to maintain circular structure */
  k_pcb_list_insert(head, to_move);
}

/**
 * @brief Create a new child process with initialized PCB.
 * @param parent The parent process ID.
 * @param pid The PID claimed for this process.
 * @param thread The spthread for this process.
 * @pre Interrupts are disabled
 * @return The process PID.
 */
pid_t k_proc_create(pid_t parent, pid_t pid, spthread_t thread) {
  ASSERT_NO_INTERRUPT("k_proc_create");
  /* Initialize all PCB fields to safe defaults */
  pcb_t* data = k_pcb_data(pid);
  data->thread = thread;
  data->parent = parent;
  data->alive_child = -1;
  data->zombie_child = -1;
  data->sched_priority = DEFAULT_SCHEDULE_PRIORITY;
  data->state = ACTIVE;
  data->wakeup_time = 0;
  data->terminated_by_signal = false;
  data->ign_int = false;
  data->ign_stop = false;
  data->stopped_as_blocked = false;
  strncpy(data->process_name, "unknown", MAX_PROCESS_NAME - 1);
  data->process_name[MAX_PROCESS_NAME - 1] = '\0';

  /* Link into parent's child list for parent-child relationship tracking. */
  if (parent > 0) {
    k_pcb_list_add(&k_pcb_data(parent)->alive_child, pid);
  }

  return pid;
}

/**
 * @brief Convert an active process to zombie state.
 * @param pid The process to zombify.
 * @param signaled true if killed by signal, false for normal exit.
 * @pre Interrupts are disabled
 */
void k_proc_zombify(pid_t pid, bool signaled) {
  ASSERT_NO_INTERRUPT("k_proc_zombify");

  /* Remove from scheduler to prevent being scheduled after termination */
  k_sched_remove(pid);

  /* Remove from signal ignore lists to clean up data structures */
  k_remove_stop(pid);
  k_remove_int(pid);
  pcb_t* data = k_pcb_data(pid);
  data->state = ZOMBIE;
  data->terminated_by_signal = signaled;

  /* Close all fds to decrement ref counts on kernel file entries. */
  if (data->fd_table != NULL) {
    for (size_t i = 0; i < vector_len(&data->fd_table); i++) {
      int gfd = data->fd_table[i];
      if (gfd != -1) {
        k_fd_close(gfd);
      }
    }
    vector_free(&data->fd_table);
    data->fd_table = NULL;
  }

  log_zombie(pid, data->sched_priority, data->process_name);

  /* Log all children as orphans */
  pid_t alive_start = data->alive_child;
  if (alive_start != -1) {
    pid_t curr = alive_start;
    do {
      log_orphan(curr, k_pcb_data(curr)->sched_priority,
                 k_pcb_data(curr)->process_name);
      curr = k_pcb_data(curr)->nex_sibling;
    } while (curr != alive_start);
  }

  pid_t zombie_start = data->zombie_child;
  if (zombie_start != -1) {
    pid_t curr = zombie_start;
    do {
      log_orphan(curr, k_pcb_data(curr)->sched_priority,
                 k_pcb_data(curr)->process_name);
      curr = k_pcb_data(curr)->nex_sibling;
    } while (curr != zombie_start);
  }

  /* Reparent children to init to prevent resource leaks. Init's main loop
   * continuously reaps zombies, ensuring orphaned zombies don't accumulate */
  pcb_t* init_data = k_pcb_data(ORPHAN_FALLBACK);
  transfer_parent(&init_data->alive_child, data->alive_child, ORPHAN_FALLBACK);
  transfer_parent(&init_data->zombie_child, data->zombie_child,
                  ORPHAN_FALLBACK);

  /* Move from parent's alive to zombie list so waitpid can find us.
   * k_notify_parent wakes parent if blocked in waitpid waiting for this child
   */
  if (data->parent > 0) {
    pcb_t* par_data = k_pcb_data(data->parent);
    k_pcb_list_remove(&par_data->alive_child, pid);
    k_pcb_list_add(&par_data->zombie_child, pid);
    k_notify_parent(pid, signaled ? W_SIGNALED : W_EXITED);
  }
}

/**
 * @brief Reap a zombie process and release its resources.
 * @param pid The zombie process to clean up.
 * @pre Interrupts are disabled
 */
void k_proc_cleanup(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_proc_cleanup");
  pcb_t* data = k_pcb_data(pid);
  data->state = WAITED;

  spthread_join(data->thread, NULL);

  /* Log with special flag if init is reaping */
  log_waited(pid, data->sched_priority, data->process_name,
             k_sched_current_pid() == 1);

  /* Remove from parent's zombie list before releasing to maintain list
   * integrity */
  if (data->parent > 0) {
    pcb_t* par_data = k_pcb_data(data->parent);
    k_pcb_list_remove(&par_data->zombie_child, pid);
  }

  /* Free PCB for reuse by future processes */
  k_proc_release(pid);
}
