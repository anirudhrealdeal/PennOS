/**
 * @file job_table.c
 * @brief Implementation of job table management functions.
 *
 * This file implements the data structure operations for the shell's
 * job table, including initialization, adding/removing jobs, and
 * status display.
 */

#include "io/term/s_term.h"
#include "job_table_t.h"
#include "util/vector.h"

/**
 * @brief Initialize a new job table.
 *
 * Creates a job table with initial capacity for 10 jobs.
 *
 * @return Initialized job_table_t structure.
 */
job_table_t init_job_table() {
  job_table_t job_table;
  job_table.curr_age = 0;
  job_table.latest_job = -1;
  job_table.jobs = vector_new(job_t, 10, NULL);
  vector_push(&job_table.jobs, (job_t){0, 0});
  return job_table;
}

/**
 * @brief Free a job table.
 *
 * @return Pointer to the job table.
 */
void free_job_table(job_table_t* job_table) {
  for (int i = 0; i < vector_len(&job_table->jobs); i++) {
    if (job_table->jobs[i].pid != -1) {
      job_remove(job_table, i);
    }
  }
  vector_free(&job_table->jobs);
}

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
int job_add(job_table_t* job_table, job_t job) {
  job.age = job_table->curr_age++;
  int insert_pos = 1;
  while (insert_pos < vector_len(&job_table->jobs) &&
         job_table->jobs[insert_pos].pid != -1) {
    insert_pos++;
  }
  if (insert_pos == vector_len(&job_table->jobs)) {
    vector_push(&job_table->jobs, job);
  } else {
    job_table->jobs[insert_pos] = job;
  }
  return insert_pos;
}

/**
 * @brief Remove a job from the table.
 *
 * Sets pid to -1 to mark the slot as empty and frees the dynamically
 * allocated command string.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to remove.
 */
void job_remove(job_table_t* job_table, int job_id) {
  job_table->jobs[job_id].pid = -1;
  free(job_table->jobs[job_id].command);
  free(job_table->jobs[job_id].cmd_info);
}

/**
 * @brief Mark a job as most recently accessed.
 *
 * Updates the job's age to the current counter value and sets latest_job
 * to this job ID.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to touch.
 */
void job_touch(job_table_t* job_table, int job_id) {
  job_table->jobs[job_id].age = job_table->curr_age++;
  job_table->latest_job = job_id;
}

/**
 * @brief Print a job's status line.
 *
 * @param job_table Pointer to the job table.
 * @param job_id Job ID to display.
 * @param status Human-readable status string.
 */
void print_job_status(job_table_t* job_table, int job_id, const char* status) {
  char ind_char = (job_id == job_table->latest_job) ? '+' : ' ';
  s_printf("[%i]%c\t%s\t\t%s\n", job_id, ind_char, status,
           job_table->jobs[job_id].command);
}
