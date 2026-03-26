/**
 * @file s_lifecycle.c
 * @brief Implementation of system-level process lifecycle operations.
 *
 * This file implements the user-facing system calls for process management:
 * spawning new processes, waiting for children, and exiting. It handles
 * the translation between user-space and kernel-space operations.
 */

#include "s_lifecycle.h"
#include "k_lifecycle.h"

#include "io/filetable/k_filetable.h"
#include "process/pcb/k_pcb.h"
#include "process/pcb/pcb_t.h"
#include "process/schedule/k_schedule.h"
#include "process/signal/k_signal.h"
#include "thread_info_t.h"

#include <string.h>
#include "util/log.h"

/**
 * @brief Entry point for newly spawned process threads.
 * @param args Pointer to thread_info_t structure.
 * @return The return value from the process's main function.
 */
void* process_spawner(void* args) {
  thread_info_t info = *((thread_info_t*)args);
  free(args);

  // Create per-process fd_table with mappings for 0,1,2
  k_disable_interrupts();
  pcb_t* pcb = k_pcb_data(info.pid);
  pcb->fd_table = vector_new(int, 4, NULL);
  vector_push(&pcb->fd_table, info.fd0);  // stdin mapping
  vector_push(&pcb->fd_table, info.fd1);  // stdout mapping
  vector_push(&pcb->fd_table, info.fd2);  // stderr mapping

  /* Terminal ownership transfer for foreground processes.
   * If parent was designated to pass terminal, claim it immediately */
  if (pcb->parent == s_get_terminal_pass_parent()) {
    s_pass_terminal_to_next_child(-1);
    s_claim_terminal(k_sched_current_pid());
  }
  k_enable_interrupts();

  /* Execute the process's main function with its argv */
  void* status = info.func(info.argv);

  /* Process returned normally, convert to zombie and notify parent */
  k_disable_interrupts();
  k_proc_zombify(info.pid, false);
  spthread_exit(status);

  /*Safety fallback but should never reach this point*/
  exit(1);
}

/**
 * @brief Spawn a new child process.
 * @param func Entry point function for the process.
 * @param name Process name for logging.
 * @param argv Argument vector.
 * @param fd0 Local stdin fd.
 * @param fd1 Local stdout fd.
 * @param fd2 Local stderr fd.
 * @return The new process's PID, or -1 on error.
 */
pid_t s_spawn(void* (*func)(void*),
              const char* name,
              char* argv[],
              int fd0,
              int fd1,
              int fd2) {
  pid_t par = k_sched_current_pid();

  /* Translate local file descriptors to kernel global file descriptors if
   * spawning from a process context. If spawning from non process context,
   * use file descriptors as is.*/
  if (par != -1) {
    k_disable_interrupts();
    fd0 = k_lfd_get(par, fd0);
    k_enable_interrupts();
    if (fd0 < 0) {
      return -1;
    }
    k_disable_interrupts();
    fd1 = k_lfd_get(par, fd1);
    k_enable_interrupts();
    if (fd1 < 0) {
      return -1;
    }
    k_disable_interrupts();
    fd2 = k_lfd_get(par, fd2);
    k_enable_interrupts();
    if (fd2 < 0) {
      return -1;
    }
  }

  /* Allocate new PID and increment reference counts on file descriptors
   * so they aren't freed when parent closes them */
  k_disable_interrupts();
  pid_t pid = k_pcb_claim();
  k_fd_dup(fd0);
  k_fd_dup(fd1);
  k_fd_dup(fd2);
  k_enable_interrupts();

  /* Prepare thread info structure to pass to process_spawner.
   * Heap-allocated because it crosses thread boundary */
  struct spthread_st thread;
  thread_info_t* args = malloc(sizeof(thread_info_t));
  args->argv = argv;
  args->func = func;
  args->pid = pid;
  args->fd0 = fd0;
  args->fd1 = fd1;
  args->fd2 = fd2;

  /* Create spthread and initialize PCB, then add to scheduler */
  k_disable_interrupts();
  spthread_create(&thread, NULL, process_spawner, args);
  k_proc_create(k_sched_current_pid(), pid, thread);

  /* Set process name for debugging and logging */
  pcb_t* data = k_pcb_data(pid);
  strncpy(data->process_name, name, MAX_PROCESS_NAME);

  log_create(pid, data->sched_priority, data->process_name);
  k_sched_add(pid);
  k_enable_interrupts();

  return pid;
}

/**
 * @brief Wait for a child process to change state.
 * @param target PID to wait for (-1 = any child).
 * @param[out] wstatus Status output.
 * @param nohang Return immediately if no child ready.
 * @return PID of child, 0 if nohang with no child ready, -1 on error.
 */
pid_t s_waitpid(pid_t target, int* wstatus, bool nohang) {
  pid_t parent = k_sched_current_pid();

  /* Check if parent has any children at all (alive or zombie).
   * If not, return error immediately */
  k_disable_interrupts();
  pcb_t* data = k_pcb_data(parent);
  if (data->alive_child == -1 && data->zombie_child == -1) {
    k_enable_interrupts();
    return -1;
  }
  /* Poll for already-terminated children without blocking */
  pid_t res = k_pollpid(parent, target, wstatus);
  k_enable_interrupts();

  /* If found a zombie or nohang requested, return immediately */
  if (res != 0 || nohang) {
    return res;
  }

  /* Prepare to block waiting for state change. Set up PCB fields that
   * k_notify_parent will use to wake us */
  k_disable_interrupts();
  int out_wstatus;
  data->waitpid_target = target;
  data->wstatus = &out_wstatus;
  k_sched_block(parent);
  k_enable_interrupts();

  /* Suspend this spthread until child signals us via k_notify_parent */
  spthread_suspend_self();

  /* Resumed by child's state change. Extract results from PCB */
  k_disable_interrupts();
  if (data->waitpid_target > 0) {
    if (wstatus) {
      *wstatus = out_wstatus;
    }
    /* If child terminated, reap the zombie */
    if (W_WIFSIGNALED(out_wstatus) || W_WIFEXITED(out_wstatus)) {
      k_proc_cleanup(data->waitpid_target);
    }
  }
  res = data->waitpid_target;
  k_enable_interrupts();

  return res;
}

/**
 * @brief Exit the current process.
 */
void s_exit() {
  pid_t cur = k_sched_current_pid();
  if (cur < 0) {
    /* If no current process just return */
    return;
  }

  /* Prevent being suspended while exiting and terminate thread */
  k_disable_interrupts();

  /* Convert to zombie: close files, reparent children, notify parent */
  k_proc_zombify(cur, false);

  /* Terminate the underlying spthread - scheduler will never resume this */
  spthread_exit(NULL);

  /* Fallback if spthread_exit somehow returns */
  exit(0);
}
