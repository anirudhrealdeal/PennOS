/**
 * @file k_pcb.c
 * @brief Implementation of kernel-level PCB table management.
 *
 * This file implements the PCB table operations for PennOS, including
 * PID allocation, release, and circular doubly-linked list operations
 * for managing parent-child sibling relationships.
 */

#include "k_pcb.h"

#include "process/pcb/pcb_table_t.h"
#include "process/schedule/k_schedule.h"

/** @brief Global PCB table instance. */
pcb_table_t k_pcb_table;

/**
 * @brief Initialize the PCB table.
 *
 * Allocates the PCB vector with default capacity and reserves PID 0
 * for the main/init process.
 *
 * @return 0 on success, -1 on failure.
 */
void k_pcb_list_add(pid_t* head, pid_t to_add) {
  ASSERT_NO_INTERRUPT("k_pcb_list_add");
  pcb_t* data = k_pcb_data(to_add);
  if (*head != -1) {
    /* Insert after head to maintain most-recent-first ordering. */
    data->prev_sibling = *head;
    data->nex_sibling = k_pcb_data(*head)->nex_sibling;

    /* Update both neighbors to point back to the new node */
    k_pcb_data(*head)->nex_sibling = to_add;
    k_pcb_data(data->nex_sibling)->prev_sibling = to_add;
  } else {
    /* Empty list becomes single-element circular list pointing to itself */
    data->nex_sibling = data->prev_sibling = to_add;
    *head = to_add;
  }
}

/**
 * @brief Remove a PID from a circular sibling list.
 *
 * Removes the process from its circular doubly-linked list and
 * resets its sibling pointers to self.
 *
 * @param[in,out] head Pointer to the list head (updated if removing head).
 * @param to_remove The PID to remove from the list.
 */
void k_pcb_list_remove(pid_t* head, pid_t to_remove) {
  ASSERT_NO_INTERRUPT("k_pcb_list_remove");
  pcb_t* data = k_pcb_data(to_remove);
  if (data->prev_sibling != to_remove) {
    /* Multi-element list: unlink by connecting neighbors to each other */
    k_pcb_data(data->prev_sibling)->nex_sibling = data->nex_sibling;
    k_pcb_data(data->nex_sibling)->prev_sibling = data->prev_sibling;

    /* Update head if we're removing the head node */
    if (*head == to_remove) {
      *head = data->prev_sibling;
    }

    /* Reset removed node to self-circular for safety in case it's accessed */
    data->prev_sibling = to_remove;
    data->nex_sibling = to_remove;
  } else {
    *head = -1;
  }
}

/**
 * @brief Insert an entire circular list into another.
 *
 * Merges two circular lists by splicing.
 *
 * @param[in,out] head Pointer to destination list head.
 * @param to_insert Head of the source list to merge in.
 */
void k_pcb_list_insert(pid_t* head, pid_t to_insert) {
  ASSERT_NO_INTERRUPT("k_pcb_list_insert");
  if (to_insert == -1)
    return;

  /* Find the cut point in to_insert's circular list. */
  pcb_t* data_end = k_pcb_data(to_insert);
  int to_insert_cut = data_end->nex_sibling;
  pcb_t* data_beg = k_pcb_data(to_insert_cut);
  if (*head != -1) {
    /* Splice to_insert ring between head and head->next. */
    pcb_t* head_data = k_pcb_data(*head);
    data_beg->prev_sibling = *head;
    data_end->nex_sibling = head_data->nex_sibling;
    head_data->nex_sibling = to_insert_cut;
    k_pcb_data(data_end->nex_sibling)->prev_sibling = to_insert;
  } else {
    /* Empty destination list: just use source list as-is */
    *head = to_insert;
  }
}

/**
 * @brief Release a PID for reuse.
 *
 * Updates avail_process hint if this PID is lower than current hint.
 *
 * @param pid The PID to release.
 */
void k_proc_release(pid_t pid) {
  if (pid < k_pcb_table.avail_process) {
    k_pcb_table.avail_process = pid;
  }
}

/**
 * @brief Claim a new PID from the PCB table.
 *
 * Finds the first available PCB slot or expands the vector if none available.
 *
 * @return The newly claimed PID.
 */
pid_t k_pcb_claim() {
  ASSERT_NO_INTERRUPT("k_pcb_claim");

  /* Start search from last known available slot to avoid redundant scanning */
  pid_t pid = k_pcb_table.avail_process;
  while (pid < vector_len(&k_pcb_table.processes)) {
    if (k_pcb_table.processes[pid].state == WAITED) {
      /* Mark as STOPPED temporarily until k_proc_create sets it to ACTIVE. */
      k_pcb_table.processes[pid].state = STOPPED;
      return pid;
    } else {
      pid++;
    }
  }

  /* No free slots found, expand the table */
  if (pid == vector_len(&k_pcb_table.processes)) {
    vector_push(&k_pcb_table.processes, (pcb_t){});
  }

  /* Move hint forward since we just allocated this slot */
  k_pcb_table.avail_process = pid + 1;
  return pid;
}

/**
 * @brief Initialize the PCB table.
 *
 * Creates the vector with DEFAULT_CAPACITY and reserves PID 0 for
 * the main process. Sets avail_process to 1.
 *
 * @return 0 on success.
 */
int k_pcb_init() {
  k_pcb_table.processes = vector_new(pcb_t, DEFAULT_CAPACITY, NULL);
  vector_push(&k_pcb_table.processes, (pcb_t){});
  k_pcb_table.avail_process = 1;
  return 0;
}

/**
 * @brief Free the PCB table when exiting
 */
void k_pcb_clean() {
  vector_free(&k_pcb_table.processes);
}

/**
 * @brief Get direct access to a process's PCB.
 *
 * Returns a pointer to the PCB for direct manipulation.
 *
 * @param pid The process ID to look up.
 * @pre Interrupts are disabled
 * @return Pointer to the PCB structure.
 */
pcb_t* k_pcb_data(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_pcb_data");
  return k_pcb_table.processes + pid;
}

/**
 * @brief Get the current size of the PCB table.
 *
 * @return Number of entries (including WAITED) in the PCB table.
 */
int k_pcb_table_size() {
  return vector_len(&k_pcb_table.processes);
}

/**
 * @brief Get the next active process PID after a given PID.
 *
 * Iterates through the PCB table to find the next non-WAITED process.
 * Used for implementing ps command.
 *
 * @param pid The starting PID (exclusive).
 * @return Next active PID, or 0 if none found.
 */
pid_t k_next_process(pid_t pid) {
  ASSERT_NO_INTERRUPT("k_next_process");
  while (pid < vector_len(&k_pcb_table.processes) &&
         k_pcb_data(pid)->state == WAITED) {
    pid++;
  }
  return (pid == vector_len(&k_pcb_table.processes)) ? 0 : pid;
}
