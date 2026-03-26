/**
 * @file s_schedule.h
 * @brief System-level scheduling interface for user processes.
 *
 * This header provides the system call interface for scheduling-related
 * operations available to user processes, including getting the current
 * PID and sleeping for a specified duration.
 */

#ifndef S_SCHEDULE_H_
#define S_SCHEDULE_H_

#include "process/pstd.h"

/**
 * @brief Suspends execution of the calling proces for a specified number of
 * clock ticks.
 *
 * This function is analogous to `sleep(3)` in Linux, with the behavior that the
 * system clock continues to tick even if the call is interrupted.
 *
 * @param ticks Duration of the sleep in system clock ticks. Must be greater
 * than 0.
 */
void s_sleep(unsigned int ticks);

/**
 * @brief Returns the current process ID.
 */
pid_t s_getpid(void);

#endif  // S_SCHEDULE_H_