/**
 * @file pcb_table_t.h
 * @brief PCB table structure for managing all processes.
 *
 * This header defines the global PCB table structure that holds all
 * process control blocks in the system. The table uses a vector for
 * dynamic growth and tracks the next available PID slot.
 */

#ifndef PCB_TABLE_H_
#define PCB_TABLE_H_

#include "pcb_t.h"
#include "util/vector.h"

/** @brief Initial capacity for the PCB table vector. */
#define DEFAULT_CAPACITY 100

/** @brief Reserved PID for the main/init process. */
#define MAIN_PROC 0

/**
 * @brief Global PCB table structure.
 *
 * Manages all PCBs in the system. PIDs are indices into the processes
 * vector. The avail_process field tracks the lowest potentially free
 * slot for efficient PID allocation.
 */
typedef struct {
  vector(pcb_t) processes;  /**< Vector of all PCBs indexed by PID. */
  pid_t avail_process;      /**< Hint for next available PID slot. */
} pcb_table_t;

#endif  // PCB_TABLE_H_