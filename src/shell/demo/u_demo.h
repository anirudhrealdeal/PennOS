#ifndef U_DEMO_H_
#define U_DEMO_H_

void* u_busy(void* arg);
void* u_zombify(void* arg);
void* u_zombie_child(void* arg);
void* u_orphanify(void* arg);
void* u_orphan_child(void* arg);

void* u_hang(void*);
void* u_nohang(void*);
void* u_recur(void*);

// this one requires the fs to hold at least 5480 bytes for a file.
void* u_crash(void*);

#endif  // U_DEMO_H_