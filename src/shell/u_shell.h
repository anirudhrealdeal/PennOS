/**
 * @file u_shell.h
 * @brief PennOS shell entry point declaration.
 *
 * This header declares the main shell loop function that provides
 * the interactive command-line interface for PennOS.
 */

#ifndef U_SHELL_H_
#define U_SHELL_H_

/**
 * @brief Main shell loop for PennOS.
 *
 * Runs the interactive shell or executes a script file.
 *
 * @param arg NULL for interactive mode, or argv array with script filename.
 * @return Always NULL.
 */
void* u_shell(void* arg);

#endif  // U_SHELL_H_
