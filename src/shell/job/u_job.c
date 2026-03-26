/**
 * @file job_control.c
 * @brief Implementation of user-level job control commands.
 *
 * This file implements the bg, fg, and jobs built-in shell commands
 * for managing background and foreground processes in PennOS.
 */

#include "u_job.h"
#include "io/term/s_term.h"
#include "job_table_t.h"
#include "process/lifecycle/s_lifecycle.h"
#include "process/pcb/s_pcb.h"
#include "process/schedule/s_schedule.h"
#include "process/signal/s_signal.h"
#include "util/vector.h"

/**
 * @brief Resume a stopped job in the background.
 *
 * If a job ID is provided, validates and uses that job. Otherwise,
 * searches for the most recently stopped job.
 *
 * @param job_table Pointer to the shell's job table.
 * @param argv NULL-terminated argument array.
 */
void u_bg(job_table_t* job_table, char** argv) {
  int job;
  pid_t pid;
  if (argv[1] != NULL) {
    /* Parse job ID from command line argument */
    job = atoi(argv[1]);

    /* Job ID must be valid (>=1), within table bounds, and reference
     * an active job */
    if (job < 1 || job >= vector_len(&job_table->jobs) ||
        job_table->jobs[job].pid == -1) {
      s_printf("Invalid job\n");
      return;
    }

    pid = job_table->jobs[job].pid;
  } else {
    /* A job is not provided, find the most recent task that is not stopped */
    job = -1;
    for (int i = 1; i < vector_len(&job_table->jobs); i++) {
      /* Job slot must be occupied and process must be stopped */
      if (job_table->jobs[i].pid != -1 &&
          s_proc_state(job_table->jobs[i].pid) == STOPPED &&

          /* Track job with highest age */
          (job == -1 || job_table->jobs[job].age < job_table->jobs[i].age)) {
        job = i;
      }
    }

    if (job == -1) {
      s_printf("No stopped jobs\n");
      return;
    }
    pid = job_table->jobs[job].pid;
  }

  /* Update age so this becomes the current job */
  job_touch(job_table, job);

  /* Send SIGCONT to resume execution without bringing to foreground */
  s_kill(pid, SIGCONT);
  s_printf("[%i] %i\n", job, pid);
}

/**
 * @brief Bring a job to the foreground.
 *
 * If a job ID is provided, validates and uses that job. Otherwise,
 * selects the most recent job (highest age, doesn't need to be stopped).
 *
 * @param job_table Pointer to the shell's job table.
 * @param argv NULL-terminated argument array.
 */
void u_fg(job_table_t* job_table, char** argv) {
  int job;
  pid_t pid;
  if (argv[1] != NULL) {
    /* A job is provided, use it if it exists */
    job = atoi(argv[1]);

    /* Validate proper job is provided */
    if (job < 1 || job >= vector_len(&job_table->jobs) ||
        job_table->jobs[job].pid == -1) {
      s_printf("Invalid job\n");
      return;
    }

    pid = job_table->jobs[job].pid;
  } else {
    /* A job is not provided, find the most recent task that is not stopped */
    job = -1;
    for (int i = 1; i < vector_len(&job_table->jobs); i++) {
      if (/* Job must exist, doesn't need to be stopped */
          job_table->jobs[i].pid != -1 &&

          /* Choose this job if it has the highest age property (so far) */
          (job == -1 || job_table->jobs[job].age < job_table->jobs[i].age)) {
        job = i;
      }
    }

    if (job == -1) {
      s_printf("No jobs\n");
      return;
    }
    pid = job_table->jobs[job].pid;
  }

  /* When we start the job, it will be the most recent job */
  job_touch(job_table, job);
  print_job_status(job_table, job, "Running");

  /* Give to-be-foreground process control of terminal */
  s_claim_terminal(pid);

  s_kill(pid, SIGCONT);

  /* Wait for job to finish or stop */
  int wstatus;
  s_waitpid(pid, &wstatus, false);
  s_claim_terminal(s_getpid());
  if (W_WIFSTOPPED(wstatus)) {
    print_job_status(job_table, job, "Stopped");
  } else {
    job_remove(job_table, job);
  }
}

/**
 * @brief List all jobs in the job table.
 *
 * @param job_table Pointer to the shell's job table.
 */
void u_jobs(job_table_t* job_table) {
  bool has_job = false;
  for (int curr_job = 1; curr_job < vector_len(&job_table->jobs); curr_job++) {
    /* If job exists, print it out */
    if (job_table->jobs[curr_job].pid != -1) {
      has_job = true;

      /* Convert status to human-readable format */
      char* state_str;
      switch (s_proc_state(job_table->jobs[curr_job].pid)) {
        case ACTIVE:
          state_str = "Running";
          break;
        case BLOCKED:
          state_str = "Blocked";
          break;
        case STOPPED:
          state_str = "Stopped";
          break;
        default:
          state_str = "Running";
          break;
      }

      print_job_status(job_table, curr_job, state_str);
    }
  }

  /* has_job remains false when we never find a job */
  if (!has_job) {
    s_printf("No jobs\n");
  }
}
