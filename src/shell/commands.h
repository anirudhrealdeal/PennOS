/**
 * @file commands.h
 * @brief Command dispatch interface for the PennOS shell.
 *
 * This header provides the main interface for executing shell commands,
 * including the run_command dispatcher, manual page display, and echo.
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "job/job_table_t.h"
#include "util/parser.h"

/**
 * @brief Execute a shell command.
 *
 * Main command dispatcher that handles built-in commands directly and
 * spawns external commands as separate processes. Handles I/O redirection,
 * background execution, and job table management.
 *
 * @param argv NULL-terminated argument array (argv[0] = command name).
 * @param cmd Parsed command structure with redirection and background flags.
 * @param nice Priority level for the command (0-2), or -1 for default.
 * @param job_table Pointer to the shell's job table for tracking background jobs.
 */
void run_command(char** argv,
                 struct parsed_command* cmd,
                 int nice,
                 job_table_t* job_table);


/**
 * @brief Display the manual/help page.
 *
 * Usage: man
 * Lists all available built-in and external commands.
 */                 
void u_man();

/**
 * @brief Echo arguments to standard output.
 *
 * Usage: echo [ARGS...]
 * Prints all arguments separated by spaces, followed by a newline.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_echo(void* arg);

#endif  // COMMANDS_H_
