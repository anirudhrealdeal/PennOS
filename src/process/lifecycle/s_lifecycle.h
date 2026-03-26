/**
 * @file s_lifecycle.h
 * @brief System-level process lifecycle interface.
 *
 * This header provides the system call interface for process lifecycle
 * operations in PennOS, including spawning new processes, waiting for
 * child termination, and voluntary process exit.
 */

#ifndef S_LIFECYCLE_H_
#define S_LIFECYCLE_H_

#include "process/pstd.h"

/**
 * @brief Create a child process that executes the function `func`.
 * The child will retain some attributes of the parent.
 *
 * @param func Function to be executed by the child process.
 * @param argv Null-terminated array of args, including the command name as
 * argv[0].
 * @param fd0 Input file descriptor.
 * @param fd1 Output file descriptor.
 * @param fd2 Error file descriptor.
 * @return pid_t The process ID of the created child process.
 */
pid_t s_spawn(void* (*func)(void*),
              const char* name,
              char* argv[],
              int fd0,
              int fd1,
              int fd2);

/**
 * @brief Unconditionally exit the calling process.
 */
void s_exit(void);

/**
 * @brief Wait on a child of the calling process, until it changes state.
 * If `nohang` is true, this will not block the calling process and return
 * immediately.
 *
 * @param pid Process ID of the child to wait for.
 * @param wstatus Pointer to an integer variable where the status will be
 * stored.
 * @param nohang If true, return immediately if no child has exited.
 * @return pid_t The process ID of the child which has changed state on success,
 * -1 on error.
 */
pid_t s_waitpid(pid_t pid, int* wstatus, bool nohang);

#endif  // S_LIFECYCLE_H_