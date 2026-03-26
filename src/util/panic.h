/**
 * @file panic.h
 * @brief Kernel panic handling for PennOS.
 *
 * This header provides the panic macro for handling unrecoverable errors.
 * When enabled (default), panic prints an error message and aborts.
 * Can be disabled by defining DISABLE_PANIC before including this header.
 */

#ifndef PANIC_H_
#define PANIC_H_

/**
 * @brief Print error message and abort the program.
 *
 * Writes the error message to stderr using s_write, then calls abort().
 * Handles partial writes and retries on EINTR/EAGAIN.
 *
 * @param error_message The error message to display.
 */
void print_and_abort(const char* error_message);

#ifdef DISABLE_PANIC

/**
 * @brief Panic macro (disabled).
 *
 * When DISABLE_PANIC is defined, panic does nothing.
 */
#define panic(error_message) ((void)0)

#else

/**
 * @brief Panic macro (enabled).
 *
 * Calls print_and_abort with the given error message, which prints
 * to stderr and aborts the program.
 *
 * @param error_message String describing the panic reason.
 */
#define panic(error_message) print_and_abort(error_message)

#endif

#endif  // PANIC_H_
