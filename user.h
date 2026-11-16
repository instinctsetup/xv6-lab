struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ---- Consolidated OS lab assignments 3-10 ----
// Assignment 8
int va2pa(int);              // Q1: translate a user virtual address (verbose)
int getmeminfo(void*);       // Q2: fill struct meminfo (see meminfo.h)
int vmprint(void);           // Q3 (also Assignment 9 Q1): print user page table
// Assignment 9
int getfaults(void);         // Q2: lazy-allocation page faults handled
// Assignment 5 (shared page + semaphores; semaphores shared with Assignment 7)
int shm_get(void);           // map the kernel shared page, returns its user address
int sem_init(int, int);      // initialise semaphore id with a value
int sem_wait(int);           // P(): decrement, block while value <= 0
int sem_signal(int);         // V(): increment, wake one waiter
int sem_destroy(int);        // extension: release a semaphore slot
// Assignment 7
int clone(void(*)(void*), void*, void*);  // Q1: kernel thread sharing pgdir
int join(void);              // Q1: wait for one thread, return its pid
// Assignment 3 / 4 (schedulers)
int set_burst(int, int);     // set estimated burst time of a pid
int set_priority(int, int);  // set priority of a pid (1 high .. 20 low)
int get_priority(int);       // read priority of a pid
int getpinfo(void*);         // fill struct pinfo (see proc.h) for scheduler metrics
// Assignment 10 (already present from the Assignment 10 repo)
// int memstats(void); int vtop(int); int pageinfo(int); int setreadonly(int);
int memstats(void);        // Assignment 10, Q1(b): print physical memory statistics
int vtop(int);             // Assignment 10, Q2(a): virtual-to-physical translation
int pageinfo(int);         // Assignment 10, Q3(a): page mapping + flags info
int setreadonly(int);      // Assignment 10, Q3(b): clear PTE_W on a page

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
