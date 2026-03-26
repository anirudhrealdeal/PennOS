/**
 * @file k_pcb.h
 * @brief Kernel-level PCB table management interface.
 *
 * This header provides the kernel-level interface for managing the
 * PCB table, including initialization, PID allocation/release, and
 * circular sibling list operations for parent-child relationships.
 */

#ifndef K_PCB_H_
#define K_PCB_H_

#include "pcb_t.h"

/**
 * @brief Initialize the PCB table.
 *
 * Allocates the PCB vector with default capacity and reserves PID 0
 * for the main/init process.
 *
 * @return 0 on success, -1 on failure.
 */
void k_pcb_list_add(pid_t* head, pid_t to_add);

/**
 * @brief Remove a PID from a circular sibling list.
 *
 * Removes the process from its circular doubly-linked list and
 * resets its sibling pointers to self.
 *
 * @param[in,out] head Pointer to the list head (updated if removing head).
 * @param to_remove The PID to remove from the list.
 */
void k_pcb_list_remove(pid_t* head, pid_t to_remove);
/**
 * @brief Insert an entire circular list into another.
 *
 * Merges two circular lists by splicing.
 *
 * @param[in,out] head Pointer to destination list head.
 * @param to_insert Head of the source list to merge in.
 */
void k_pcb_list_insert(pid_t* head, pid_t to_insert);

/**
 * @brief Release a PID for reuse.
 *
 * Updates avail_process hint if this PID is lower than current hint.
 *
 * @param pid The PID to release.
 */
void k_proc_release(pid_t pid);

/**
 * @brief Claim a new PID from the PCB table.
 *
 * Finds the first available PCB slot or expands the vector if none available.
 *
 * @return The newly claimed PID.
 */
pid_t k_pcb_claim();

/**
 * @brief Initialize the PCB table.
 *
 * Creates the vector with DEFAULT_CAPACITY and reserves PID 0 for
 * the main process. Sets avail_process to 1.
 *
 * @return 0 on success.
 */
int k_pcb_init();

/**
 * @brief Free the PCB table when exiting
 */
void k_pcb_clean();

/**
 * @brief Get direct access to a process's PCB.
 *
 * Returns a pointer to the PCB for direct manipulation.
 *
 * @param pid The process ID to look up.
 * @pre Interrupts are disabled
 * @return Pointer to the PCB structure.
 */
pcb_t* k_pcb_data(pid_t pid);

/**
 * @brief Get the current size of the PCB table.
 *
 * @return Number of entries (including WAITED) in the PCB table.
 */
int k_pcb_table_size();

/**
 * @brief Get the next active process PID after a given PID.
 *
 * Iterates through the PCB table to find the next non-WAITED process.
 * Used for implementing ps command.
 *
 * @param pid The starting PID (exclusive).
 * @return Next active PID, or 0 if none found.
 */
pid_t k_next_process(pid_t pid);

#endif  // K_PCB_H_