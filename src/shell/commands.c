/**
 * @file commands.c
 * @brief Implementation of the PennOS shell command dispatcher.
 *
 * This file implements command lookup, built-in execution, and process
 * spawning for the shell. It handles I/O redirection, background jobs,
 * and priority setting for spawned processes.
 */

#include "commands.h"

#include <string.h>
#include "io/term/s_term.h"
#include "process/lifecycle/s_lifecycle.h"
#include "process/pcb/s_pcb.h"
#include "process/schedule/s_schedule.h"

#include "demo/u_demo.h"
#include "file/u_file.h"
#include "job/u_job.h"
#include "process/u_process.h"
#include "u_shell.h"

/** @brief Function pointer type for shell command handlers. */
typedef void* (*shell_func)(void*);

/**
 * @brief Flatten an argv array into a single space-separated string.
 * @param argv NULL-terminated argument array.
 * @return Dynamically allocated string (caller must free).
 */
char* flatten_args(char** argv) {
  /* Calculate total size needed to avoid reallocation */
  int totsz = 0;
  for (int i = 0; argv[i] != NULL; i++) {
    totsz += strlen(argv[i]) + 1;
  }
  char* str = malloc(totsz);
  int p = 0;
  /* Copy each argument and add space separator */
  for (int i = 0; argv[i] != NULL; i++) {
    int len = strlen(argv[i]);
    strncpy(str + p, argv[i], len);
    p += len + 1;
    str[p - 1] = ' ';
  }
  /* Replace final space with null terminator */
  str[p - 1] = '\0';
  return str;
}

/**
 * @brief Execute shell built-in commands.
 *
 * Checks if argv[0] is a built-in command and executes it directly
 * in the shell process (not spawned). Built-ins include:
 * - nice, nice_pid: priority control
 * - man: help
 * - bg, fg, jobs: job control
 * - logout: system shutdown
 *
 * @param job_table Pointer to the shell's job table.
 * @param argv NULL-terminated argument array.
 * @param cmd Parsed command structure.
 * @return true if a built-in was executed, false otherwise.
 */
bool execute_builtins(job_table_t* job_table,
                      char** argv,
                      struct parsed_command* cmd) {
  if (strcmp(argv[0], "nice") == 0) {
    u_nice(job_table, argv, cmd);
  } else if (strcmp(argv[0], "nice_pid") == 0) {
    u_nice_pid(argv);
    free(cmd);
  } else if (strcmp(argv[0], "man") == 0) {
    u_man();
    free(cmd);
  } else if (strcmp(argv[0], "bg") == 0) {
    u_bg(job_table, argv);
    free(cmd);
  } else if (strcmp(argv[0], "fg") == 0) {
    u_fg(job_table, argv);
    free(cmd);
  } else if (strcmp(argv[0], "jobs") == 0) {
    u_jobs(job_table);
    free(cmd);
  } else if (strcmp(argv[0], "logout") == 0) {
    free(cmd);
    free_job_table(job_table);
    u_logout();
    exit(1);
  } else {
    return false;
  }
  return true;
}

/**
 * @brief Look up a command name and return its handler function.
 *
 * Maps command names to their corresponding function pointers.
 * Supported commands: cat, sleep, busy, echo, ls, touch, mv, cp, rm,
 * chmod, ps, kill, zombify, orphanify, hang, nohang, recur, crash.
 *
 * @param name Command name to look up.
 * @return Function pointer to command handler, or NULL if not found.
 */
shell_func lookup_command(const char* name) {
  if (strcmp(name, "cat") == 0) {
    return u_cat;
  } else if (strcmp(name, "sleep") == 0) {
    return u_sleep;
  } else if (strcmp(name, "busy") == 0) {
    return u_busy;
  } else if (strcmp(name, "echo") == 0) {
    return u_echo;
  } else if (strcmp(name, "ls") == 0) {
    return u_ls;
  } else if (strcmp(name, "touch") == 0) {
    return u_touch;
  } else if (strcmp(name, "mv") == 0) {
    return u_mv;
  } else if (strcmp(name, "cp") == 0) {
    return u_cp;
  } else if (strcmp(name, "rm") == 0) {
    return u_rm;
  } else if (strcmp(name, "chmod") == 0) {
    return u_chmod;
  } else if (strcmp(name, "ps") == 0) {
    return u_ps;
  } else if (strcmp(name, "kill") == 0) {
    return u_kill;
  } else if (strcmp(name, "zombify") == 0) {
    return u_zombify;
  } else if (strcmp(name, "orphanify") == 0) {
    return u_orphanify;
  } else if (strcmp(name, "hang") == 0) {
    return u_hang;
  } else if (strcmp(name, "nohang") == 0) {
    return u_nohang;
  } else if (strcmp(name, "recur") == 0) {
    return u_recur;
  } else if (strcmp(name, "crash") == 0) {
    return u_crash;
  } else {
    return NULL;
  }
}

/**
 * @brief Execute a command as a separate process.
 * @param argv NULL-terminated argument array.
 * @param cmd Parsed command with redirection/background flags.
 * @param nice Priority (0-2) or -1 for default.
 * @param job_table Pointer to shell's job table.
 */
void run_command(char** argv,
                 struct parsed_command* cmd,
                 int nice,
                 job_table_t* job_table) {
  /* Spawn as separate process */
  if (execute_builtins(job_table, argv, cmd)) {
    return;
  }

  /* Look up command function, or treat as executable script file if not found
   */
  void* (*func)(void*) = lookup_command(argv[0]);
  if (func == NULL) {
    int fd_targ = s_open(argv[0], F_EXEC);
    if (fd_targ < 0) {
      free(cmd);
      return;
    }
    /* If the file as an executable, try to execute it */
    s_close(fd_targ);
    func = u_shell;
  }

  /* Handle input redirection */
  int fd_in = STDIN_FILENO;  // stdin
  if (cmd->stdin_file != NULL) {
    fd_in = s_open(cmd->stdin_file, F_READ);
    if (fd_in < 0) {
      return;
    }
  }

  /* Handle output redirection */
  int fd_out = STDOUT_FILENO;  // stdout
  if (cmd->stdout_file != NULL) {
    int mode = cmd->is_file_append ? F_APPEND : F_WRITE;
    fd_out = s_open(cmd->stdout_file, mode);
    if (fd_out < 0) {
      if (fd_in != STDIN_FILENO) {
        s_close(fd_in);
      }
      return;
    }
  }

  if (!cmd->is_background) {
    s_pass_terminal_to_next_child(s_getpid());
  }

  job_t job;
  job.command = flatten_args(argv);
  job.cmd_info = cmd;

  /* Spawn the process with redirected file descriptors */
  pid_t pid = s_spawn(func, argv[0], argv, fd_in, fd_out, STDERR_FILENO);

  /* Close parent's copies of file descriptors */
  if (fd_in != STDIN_FILENO) {
    s_close(fd_in);
  }
  if (fd_out != STDOUT_FILENO) {
    s_close(fd_out);
  }

  if (pid == -1) {
    s_perror("Error spawning process\n");
    return;
  }
  if (nice != -1 && s_nice(pid, nice) == -1) {
    s_perror("nice: failed to set priority for PID %d\n", pid);
  }
  if (!cmd->is_background) {
    /* Wait for foreground process */
    int status;
    s_waitpid(pid, &status, false);
    s_claim_terminal(s_getpid());
    if (W_WIFSTOPPED(status)) {
      job.pid = pid;
      int job_id = job_add(job_table, job);
      job_touch(job_table, job_id);
      print_job_status(job_table, job_id, "Stopped");
    } else {
      free(job.command);
      free(job.cmd_info);
    }
  } else {
    job.pid = pid;
    int job_id = job_add(job_table, job);
    job_touch(job_table, job_id);
    s_fprintf(STDOUT_FILENO, "[%i] %i\n", job_id, pid);
  }
}

/**
 * @brief Display available commands.
 *
 * Prints a categorized list of all shell commands to stdout.
 */
void u_man() {
  s_printf("Available commands:\n");
  s_printf("man\n");
  s_printf("cat FILE ... [-(a|w) OUT_FILE] \n");
  s_printf("sleep TICKS\n");
  s_printf("busy\n ");
  s_printf("echo  [ARG]\n");
  s_printf("ls [FILE]\n");
  s_printf("touch FILE ...\n");
  s_printf("mv SOURCE DEST\n");
  s_printf("cp [-h] SOURCE [-h] DEST\n");
  s_printf("rm FILE\n ");
  s_printf("chmod PERMISSIONS FILE\n ");
  s_printf("nice PRIORITY COMMAND [COMMAND_ARGS ...]\n");
  s_printf("nice_pid PRIORITY PID\n");
  s_printf("man\n");
  s_printf("bg [JOB]\n");
  s_printf("fg [JOB]\n");
  s_printf("jobs\n");
  s_printf("logout\n");
  s_printf("kill [-term|-stop|-cont] [PID ...]\n");
  s_printf("zombify\n");
  s_printf("orphanify\n");
  s_printf("hang\n");
  s_printf("nohang\n");
  s_printf("recur\n");
  s_printf("crash\n");
}

/**
 * @brief Echo arguments to stdout.
 *
 * Prints argv[1..n] separated by spaces, ending with a newline.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_echo(void* arg) {
  char** argv = (char**)arg;

  for (int i = 1; argv[i] != NULL; i++) {
    s_fprintf(STDOUT_FILENO, "%s", argv[i]);
    if (argv[i + 1] != NULL) {
      s_fprintf(STDOUT_FILENO, " ");
    }
  }
  s_fprintf(STDOUT_FILENO, "\n");

  return NULL;
}
