#include "u_demo.h"

#include "io/term/s_term.h"
#include "process/lifecycle/s_lifecycle.h"

/* zombify - test zombie handling */
void* u_zombify(void* arg) {
  (void)arg;

  s_spawn(u_zombie_child, "zombie_child", NULL, STDIN_FILENO, STDOUT_FILENO,
          STDERR_FILENO);

  /* Infinite loop - parent never waits */
  while (1)
    ;

  return NULL;
}

/* busy - busy wait indefinitely */
void* u_busy(void* arg) {
  (void)arg;
  while (1) {
    /* Busy wait */
  }
  return NULL;
}

/* zombie_child - helper for zombify */
void* u_zombie_child(void* arg) {
  (void)arg;
  /* Just return - becomes zombie */
  return NULL;
}

/* orphanify - test orphan handling */
void* u_orphanify(void* arg) {
  (void)arg;

  s_spawn(u_orphan_child, "orphan_child", NULL, STDIN_FILENO, STDOUT_FILENO,
          STDERR_FILENO);

  /* Return immediately - child becomes orphan */
  return NULL;
}

/* orphan_child - helper for orphanify */
void* u_orphan_child(void* arg) {
  (void)arg;

  /* Infinite loop */
  while (1)
    ;

  return NULL;
}
