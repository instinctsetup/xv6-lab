// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_memstats    22  // Assignment 10, Q1(b)
#define SYS_vtop        23  // Assignment 10, Q2(a)
#define SYS_pageinfo    24  // Assignment 10, Q3(a)
#define SYS_setreadonly 25  // Assignment 10, Q3(b)

// ---- Consolidated OS lab assignments 3-10 (practice build) ----
#define SYS_va2pa        26  // Assignment 8, Q1: virtual-to-physical translation
#define SYS_getmeminfo   27  // Assignment 8, Q2: physical memory statistics
#define SYS_vmprint      28  // Assignment 8 Q3 + Assignment 9 Q1: print user page table
#define SYS_getfaults    29  // Assignment 9, Q2: lazy-allocation fault count
#define SYS_shm_get      30  // Assignment 5: shared page between parent and child
#define SYS_sem_init     31  // Assignment 7 Q2 + Assignment 5: semaphore init
#define SYS_sem_wait     32  // Assignment 7 Q2 + Assignment 5: semaphore wait (P)
#define SYS_sem_signal   33  // Assignment 7 Q2 + Assignment 5: semaphore signal (V)
#define SYS_sem_destroy  34  // Extension: free a semaphore slot and wake waiters
#define SYS_clone        35  // Assignment 7, Q1: create kernel thread sharing pgdir
#define SYS_join         36  // Assignment 7, Q1: wait for a thread to finish
#define SYS_set_burst    37  // Assignment 3 Q3 / Assignment 4 Q1: set estimated burst
#define SYS_set_priority 38  // Assignment 3 Q2 / Assignment 7 Q4: set priority
#define SYS_get_priority 39  // Assignment 3 Q2 / Assignment 7 Q4: read priority
#define SYS_getpinfo     40  // Scheduler observability for A3/A4 (times + queue level)
