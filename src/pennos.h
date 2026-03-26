/**
 * @file pennos.h
 * @brief PennOS kernel initialization and INIT process interface.
 *
 * This header declares the INIT process entry point. INIT is PID 1 and
 * serves as the ancestor of all user processes in PennOS. It is responsible
 * for spawning the shell and reaping orphaned/zombie children.
 */

#ifndef PENNOS_H_
#define PENNOS_H_

/**
 * @brief INIT process main function.
 *
 * INIT is the first user-space process started by PennOS (PID 1).
 * Its responsibilities include:
 * - Spawning the interactive shell process
 * - Adopting orphaned processes when their parents exit
 * - Reaping zombie children to prevent resource leaks
 * - Coordinating graceful shutdown when the shell exits
 *
 * INIT runs in an infinite loop calling s_waitpid(-1) to reap any
 * children that become zombies. It exits only when the shell terminates.
 *
 * @param arg Unused parameter (required by spthread signature).
 * @return NULL (never returns during normal operation).
 */
void* u_init_main(void* arg);

#endif  // PENNOS_H_
