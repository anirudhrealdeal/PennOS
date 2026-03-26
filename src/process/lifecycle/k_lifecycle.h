/**
 * @file k_lifecycle.h
 * @brief Kernel-level process lifecycle management.
 *
 * This header provides the kernel-level interface for managing process
 * lifecycles in PennOS, including process creation, termination (zombification),
 * and cleanup (reaping). These functions manipulate PCB state directly and
 * must be called with interrupts disabled.
 */

#ifndef K_LIFECYCLE_H_
#define K_LIFECYCLE_H_

#include "process/pcb/pcb_t.h"
#include "process/pstd.h"
#include "util/spthread.h"

/**
 * @brief Create an spthread for a new process.
 *
 * Creates the underlying spthread that will execute the process's code.
 * This is a low-level function used internally by process creation.
 *
 * @param func The entry point function for the thread.
 * @param argv Null-terminated argument array for the process.
 * @param fd0 File descriptor for stdin.
 * @param fd1 File descriptor for stdout.
 * @return The created spthread structure.
 */
spthread_t k_spthread_create(void* (*func)(void*),
                             char* argv[],
                             int fd0,
                             int fd1);

/**
 * @brief Create a new child process, inheriting applicable properties from the
 * parent.
 *
 * @return Reference to the child PCB.
 */
pid_t k_proc_create(pid_t parent, pid_t child, spthread_t thread);

/**
 * @brief Clean up a terminated/finished thread's resources.
 * This may include freeing the PCB, handling children, etc.
 */
void k_proc_cleanup(pid_t proc);

/**
 * @brief Convert an active process to a zombified process.
 */
void k_proc_zombify(pid_t proc, bool signaled);

/**
 * @brief Obtain the PCB data associated with a given pid.
 * There should be no interrupts while holding this pcb data
 * else it may become stale or cause a race condition.
 */
pcb_t* k_pcb_data(pid_t pid);

#endif  // K_LIFECYCLE_H_