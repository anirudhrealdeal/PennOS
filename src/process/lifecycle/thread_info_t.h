/**
 * @file thread_info_t.h
 * @brief Thread initialization information structure.
 *
 * This header defines the structure used to pass initialization data
 * to newly created process threads. It contains all information needed
 * to set up a new process's execution context.
 */

#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#include "util/spthread.h"

/**
 * @brief Information passed to a newly spawned process thread.
 *
 * This structure is allocated by the parent process and passed to
 * the child's thread entry point (process_spawner). The child is
 * responsible for freeing this structure after extracting the data.
 */
typedef struct thread_info_st {
  pid_t pid;
  void* (*func)(void*);
  char** argv;
  int fd0, fd1, fd2;
} thread_info_t;

#endif