/**
 * @file panic.c
 * @brief Implementation of kernel panic handling.
 *
 * This file implements the print_and_abort function used by the panic
 * macro to handle unrecoverable errors in PennOS.
 */

#include "./panic.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "io/term/s_term.h"


/**
 * @brief Print an error message to stderr and abort.
 *
 * Writes the error message to STDERR_FILENO using s_write,
 * handling partial writes and retrying on EINTR/EAGAIN.
 * After the message is written (or write fails), calls abort()
 * to terminate the program.
 *
 * @param error_message The message to print before aborting.
 */
void print_and_abort(const char* error_message) {
  ssize_t len = (ssize_t)strlen(error_message);
  ssize_t total = 0;
  while (total != len) {
    ssize_t res = s_write(STDERR_FILENO, error_message, len);
    if (res == 0) {
      break;
    }
    if (res == -1) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      break;
    }
    total += res;
  }

  abort();
}
