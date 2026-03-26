/**
 * @file s_term.h
 * @brief System-level terminal I/O and ownership interface.
 *
 * This header provides the system call interface for terminal operations
 * in PennOS. It includes functions for terminal ownership management
 * (job control) and formatted output macros for printing to the terminal.
 *
 * @note The s_fprintf/s_printf/s_perror macros provide printf-style
 *       formatted output using the PennOS file system calls.
 */

#ifndef S_TERM_H_
#define S_TERM_H_

#include <stdio.h>
#include <stdlib.h>
#include "io/file/s_file.h"
#include "process/pstd.h"

/**
 * @brief Set the terminal owner to the specified process.
 *
 * System call wrapper that safely claims terminal ownership with
 * interrupts disabled.
 *
 * @param pid The process ID to grant terminal ownership.
 */
void s_claim_terminal(pid_t pid);

/**
 * @brief Get the parent process that should pass terminal to its child.
 *
 * @return The parent PID, or -1 if none is set.
 */
pid_t s_get_terminal_pass_parent();

/**
 * @brief Mark a parent to pass terminal ownership to its next child.
 *
 * System call wrapper that safely sets the terminal pass parent with
 * interrupts disabled.
 *
 * @param parent The parent process ID that will pass terminal ownership.
 */
void s_pass_terminal_to_next_child(pid_t parent);

/**
 * @brief Get the current terminal owner.
 *
 * @return The terminal owner's PID, or -1 if no owner is set.
 */
pid_t s_get_terminal_owner();


/**
 * @brief Formatted print to a file descriptor.
 *
 * Printf-style formatted output that writes to the specified file descriptor
 * using s_write(). Allocates a temporary buffer for formatting.
 *
 * @param fd The file descriptor to write to.
 * @param fmt The printf-style format string.
 * @param ... Variable arguments for format string.
 * @return Number of bytes written, or -1 on error.
 */
#define s_fprintf(fd, fmt, ...)                                              \
  ({                                                                         \
    int __sp_nbytes = snprintf(NULL, 0, fmt __VA_OPT__(, ) __VA_ARGS__);     \
    char* __sp_buf = malloc(__sp_nbytes + 2);                                \
    __sp_nbytes =                                                            \
        snprintf(__sp_buf, __sp_nbytes + 2, fmt __VA_OPT__(, ) __VA_ARGS__); \
    int res = s_write(fd, __sp_buf, __sp_nbytes);                            \
    free(__sp_buf);                                                          \
    res;                                                                     \
  })

/**
 * @brief Formatted print to standard output.
 *
 * Convenience macro that calls s_fprintf with STDOUT_FILENO.
 *
 * @param ... Printf-style format string and arguments.
 * @return Number of bytes written, or -1 on error.
 */  
#define s_printf(...) s_fprintf(STDOUT_FILENO, __VA_ARGS__)

/**
 * @brief Formatted print to standard error.
 *
 * Convenience macro that calls s_fprintf with STDERR_FILENO.
 * Used for error messages.
 *
 * @param ... Printf-style format string and arguments.
 * @return Number of bytes written, or -1 on error.
 */
#define s_perror(...) s_fprintf(STDERR_FILENO, __VA_ARGS__)

#endif  // S_TERM_H_