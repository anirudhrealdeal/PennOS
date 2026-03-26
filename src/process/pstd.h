/**
 * @file pstd.h
 * @brief Process standard definitions and constants.
 *
 * This header defines constants used throughout PennOS for process management
 * related to signaling and waitpid.
 */

#ifndef PSTD_H_
#define PSTD_H_

typedef int pid_t;

// Copy the same defines as signal.h, if signal isn't included elsewhere
// SIGINT is added so we can handle Ctrl+C terminal commands
#ifndef SIGINT
#define SIGINT 2
#endif
#ifndef SIGTERM
#define SIGTERM 15
#endif
#ifndef SIGCONT
#define SIGCONT 18
#endif
#ifndef SIGSTOP
#define SIGSTOP 19
#endif

/*
 * Wait status values for s_waitpid wstatus parameter.
 * These indicate how a child process changed state.
 */
#define W_EXITED 1    /* Child exited normally (returned from function) */
#define W_SIGNALED 2  /* Child was terminated by a signal (e.g., S_SIGTERM) */
#define W_STOPPED 4   /* Child was stopped by a signal (e.g., S_SIGSTOP) */
#define W_CONTINUED 8 /* Child was continued after being stopped */

/*
 * Macros to check wait status (similar to POSIX macros)
 */
#define W_WIFEXITED(status) ((status) == W_EXITED)
#define W_WIFSIGNALED(status) ((status) == W_SIGNALED)
#define W_WIFSTOPPED(status) ((status) == W_STOPPED)
#define W_WIFCONTINUED(status) ((status) == W_CONTINUED)

#endif  // PSTD_H_