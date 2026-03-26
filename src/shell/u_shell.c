/**
 * @file shell.c
 * @brief Implementation of the PennOS interactive shell.
 *
 * This file implements the main shell loop, including input parsing,
 * command execution, job table management, and zombie process cleanup.
 * The shell supports both interactive mode and script execution.
 */

#include "u_shell.h"

/* System Functions */
#include "io/term/s_term.h"
#include "process/lifecycle/s_lifecycle.h"
#include "process/schedule/s_schedule.h"

/* User Functions */
#include "process/u_process.h"

/* Utility Headers */
#include <string.h>
#include "commands.h"
#include "util/parser.h"

/** @brief Maximum length for a single input line. */
#define MAX_INPUT 1024

/**
 * @brief Main shell loop for PennOS.
 *
 * Runs the interactive shell or executes a script file.
 *
 * @param arg NULL for interactive mode, or argv array with script filename.
 * @return Always NULL.
 */
void* u_shell(void* arg) {
  int cmd_fd = STDIN_FILENO;

  /* Determine mode from argument presence */
  bool is_main_shell = arg == NULL;
  if (!is_main_shell) {
    /* Read commands from file instead of stdin */
    char** argv = arg;
    if (argv[0] == NULL) {
      s_perror("shell: invalid arguments");
      return NULL;
    }
    /* Open script file with F_EXEC mode */
    cmd_fd = s_open(argv[0], F_EXEC);
    if (cmd_fd < 0) {
      return NULL;
    }
  }

  /* Initialize job table for tracking background processes */
  job_table_t shell = init_job_table();

  char input[MAX_INPUT];

  while (1) {
    /* Ensure shell controls the terminal when idle */
    s_claim_terminal(s_getpid());

    if (is_main_shell) {
      s_fprintf(STDOUT_FILENO, "$ ");
    }

    int input_len = 0;
    int num_bytes;
    /* Read input character-by-character rather than line-buffered. */
    while (input_len + 1 < MAX_INPUT &&
           (num_bytes = s_read(cmd_fd, input + input_len, 1)) == 1) {
      input_len++;
      if (input[input_len - 1] == '\n') {
        break;
      }
    }
    input[input_len] = '\0';

    if (num_bytes < 0) {
      s_perror("shell: failed to read input\n");
    }

    if (input_len == 0) {
      /* EOF detected. Interactive mode triggers system shutdown, script mode
       * just exits */
      free_job_table(&shell);

      /* EOF (Ctrl-D) - shutdown PennOS */
      if (is_main_shell) {
        s_printf("\n");
        u_logout();
      }
      return NULL;
    }
    input[input_len] = '\0';

    /* Reap completed background jobs before processing next command.
     * WNOHANG prevents blocking, so we only clean up jobs that already finished
     */
    int status;
    pid_t done_pid;
    while ((done_pid = s_waitpid(-1, &status, true)) > 0) {
      /* Find corresponding job in job table by matching PID */
      int job = 1;
      while (job < vector_len(&shell.jobs) && shell.jobs[job].pid != done_pid) {
        job++;
      }
      /* Skip if PID wasn't found in job table (shouldn't happen but defensive)
       */
      if (job == vector_len(&shell.jobs)) {
        continue;
      }
      print_job_status(&shell, job, "Done");
      job_remove(&shell, job);
      /* Update latest_job pointer if we just removed the current job.
       * Scan remaining jobs to find the one with highest age */
      if (job == shell.latest_job) {
        shell.latest_job = -1;
        for (int i = 1; i < vector_len(&shell.jobs); i++) {
          if (shell.jobs[i].pid != -1 &&
              (shell.latest_job == -1 ||
               shell.jobs[shell.latest_job].age < shell.jobs[i].age)) {
            shell.latest_job = i;
          }
        }
      }
    }

    /* Strip trailing newline for cleaner parsing */
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
      input[len - 1] = '\0';
    }

    /* Skip empty input */
    if (strlen(input) == 0) {
      continue;
    }

    /* Parse input */
    struct parsed_command* cmd;
    int parse_result = parse_command(input, &cmd);

    if (parse_result != PARSER_SUCCESS) {
      if (parse_result == -1) {
        s_perror("parse error");
      } else {
        print_parser_errcode(STDERR_FILENO, parse_result);
      }
      continue;
    }

    /* Validate parsed command has at least one command with one argument.
     * Empty parses can occur with pure whitespace or comments */
    if (cmd->num_commands == 0 || cmd->commands[0] == NULL ||
        cmd->commands[0][0] == NULL) {
      free(cmd);
      continue;
    }

    /* Execute the parsed command */
    char** argv = cmd->commands[0];
    run_command(argv, cmd, -1, &shell);
  }

  return NULL;
}
