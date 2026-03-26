/**
 * @file pcb_state_t.h
 * @brief Process state enumeration for PCB management.
 *
 * This header defines the possible states a process can be in during
 * its lifecycle in PennOS.
 */

#ifndef PCB_STATE_H_
#define PCB_STATE_H_

/**
 * @brief Enumeration of process states.
 *
 * - **ACTIVE**: Process is ready to run or currently running.
 * - **BLOCKED**: Process is waiting for an event (sleep, waitpid, I/O).
 * - **STOPPED**: Process has been stopped by a signal (S_SIGSTOP).
 * - **ZOMBIE**: Process has terminated but not yet reaped by parent.
 * - **WAITED**: Process has been reaped; PCB slot is available for reuse.
 */
typedef enum { ACTIVE, BLOCKED, STOPPED, ZOMBIE, WAITED } pcb_state_t;

#endif  // PCB_STATE_H_