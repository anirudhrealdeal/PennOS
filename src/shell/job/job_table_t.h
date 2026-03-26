/**
 * @file job_table_t.h
 * @brief Job table data structures and management functions.
 *
 * This header defines the data structures for tracking background jobs
 * in the PennOS shell, along with functions for adding, removing, and
 * displaying job entries.
 */

#ifndef JOB_TABLE_T_H_
#define JOB_TABLE_T_H_

#include "util/vector.h"

/**
 * @brief Represents a single job entry in the job table.
 */
typedef struct job_st {
  pid_t pid;     /**< Process ID of the job, or -1 if slot is empty. */
  int age;       /**< Age counter for determining most recent job. */
  char* command; /**< Command string that started this job. */
  struct parsed_command* cmd_info; /**< Parsed command information. */
} job_t;

typedef struct job_table_st {
  vector(job_t) jobs; /**< Vector of job entries (index 0 unused). */
  int curr_age;       /**< Monotonically increasing age counter. */
  int latest_job;     /**< Job ID of the most recently touched job. */
} job_table_t;

/**
 * @brief Initialize a new job table.
 *
 * Creates an empty job table with a placeholder at index 0
 * (job IDs start at 1).
 *
 * @return Initialized job_table_t structure.
 */
job_table_t init_job_table();

/**
 * @brief Free a job table.
 *
 * @return Pointer to the job table.
 */
void free_job_table(job_table_t* job_table);

/**
 * @brief Add a new job to the table.
 *
 * Assigns the current age to the job and finds the first empty slot
 * (pid == -1) to reuse, or appends to the end.
 *
 * @param job_table Pointer to the job table.
 * @param job Job entry with pid and command set.
 * @return Job ID of the inserted job.
 */
int job_add(job_table_t* job_table, job_t job);

/**
 * @brief Remove a job from the table.
 *
 * Sets pid to -1 to mark the slot as empty and frees the dynamically
 * allocated command string.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to remove.
 */
void job_remove(job_table_t* job_table, int job_id);

/**
 * @brief Mark a job as most recently accessed.
 *
 * Updates the job's age to the current counter value and sets latest_job
 * to this job ID.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to touch.
 */
void job_touch(job_table_t* job_table, int job_id);

/**
 * @brief Print a job's status line.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to display.
 * @param status Human-readable status string.
 */
void print_job_status(job_table_t* job_table, int job_id, const char* status);

#endif  // JOB_TABLE_T_H