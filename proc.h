// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  // ---- Consolidated OS lab assignments 3-10 (practice build) ----
  // Scheduler fields (Assignments 3 and 4; see sched.h for the policy).
  uint ctime;                  // A3 Q1/A4 Q2: creation time in ticks
  int burst_time;              // A3 Q3/A4: estimated burst, -1 = unset
  int remaining_time;          // A4 Q1 (SRTF): remaining estimated burst
  int priority;                // A3 Q2/A7 Q4: 1 = highest .. 20 = lowest
  uint ready_since;            // aging + HRRN: tick when last became RUNNABLE
  uint heap_start;             // A8 Q4 (lazy): lowest valid lazy heap address
  uint last_run;               // RR tie-break: tick of last dispatch
  uint rtime;                  // total run ticks, charged by scheduler()
  int queue_level;             // A4 Q3 (MLFQ): current queue, 0 = top
  int qticks;                  // A4 Q3 (MLFQ): ticks used at current level
  // Assignment 7 Q1 (threads).
  int is_thread;               // 1 = kernel thread created by clone()
  struct proc *leader;         // thread-group leader (self when not a thread)
  int nthreads;                // leader only: live threads in the group
  // Assignment 7 Q3: semaphore id this process is blocked on (-1 = none).
  int waiting_on;
  // Assignment 9 Q2: lazy-allocation page faults handled for this process.
  uint pgfaults;
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
