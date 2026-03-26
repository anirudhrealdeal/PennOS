/**
 * @file u_job.h
 * @brief User-level job control commands for the PennOS shell.
 *
 * This header declares the built-in shell commands for job control,
 * allowing users to manage background and foreground processes.
 */

#ifndef U_JOB_CONTROL_H_
#define U_JOB_CONTROL_H_

#include "job_table_t.h"

/**
 * @brief Resume a stopped job in the background.
 *
 * Usage: bg [JOB_ID]
 * If JOB_ID is provided, resumes that specific job. Otherwise, resumes
 * the most recently stopped job. Sends SIGCONT to the target process.
 *
 * @param job_table Pointer to the shell's job table.
 * @param argv NULL-terminated argument array (argv[1] = optional job ID).
 */
void u_bg(job_table_t* shell, char** argv);

/**
 * @brief Bring a job to the foreground.
 *
 * Usage: fg [JOB_ID]
 * If JOB_ID is provided, foregrounds that specific job. Otherwise,
 * foregrounds the most recent job. Claims the terminal, sends SIGCONT,
 * and waits for the job to complete or stop.
 *
 * @param job_table Pointer to the shell's job table.
 * @param argv NULL-terminated argument array (argv[1] = optional job ID).
 */
void u_fg(job_table_t* shell, char** argv);

/**
 * @brief List all current jobs.
 *
 * Usage: jobs
 * Displays all jobs in the job table with their status (Running, Blocked,
 * or Stopped), job ID, and command string.
 *
 * @param job_table Pointer to the shell's job table.
 */
void u_jobs(job_table_t* shell);

#endif  // U_JOB_CONTROL_H_