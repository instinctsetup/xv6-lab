#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "sched.h" // consolidated lab build
#include "meminfo.h"
#include "pinfo.h"

// Assignment 10, Q1(a): allocation/free counters maintained in kalloc.c
extern int kpage_allocs;
extern int kpage_frees;
// first address after the kernel image; defined by kernel.ld
extern char end[];

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
#if LAZY_ALLOC
  // Assignment 8 Q4 / Assignment 9 Q2: lazy allocation. Growth only
  // records the new size; frames arrive on demand in trap.c's T_PGFLT
  // handler. Shrinking still frees the pages that were really allocated
  // (growproc's deallocuvm skips not-yet-backed lazy pages), per A8 Q4(a).
  if(n < 0){
    if((0U - (uint)n) > myproc()->sz - myproc()->heap_start)
      return -1;               // never shrink below the code/guard region
    return growproc(n) < 0 ? -1 : addr;
  }
  if((uint)n >= KERNBASE - myproc()->sz)
    return -1;                 // keep the user space below KERNBASE
  myproc()->sz += n;
  return addr;
#else
  if(growproc(n) < 0)
    return -1;
  return addr;
#endif
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Assignment 10, Q1(b): display physical-memory statistics gathered by
// the kalloc()/kfree() counters in kalloc.c.
int
sys_memstats(void)
{
  int total_alloc = kpage_allocs;              // total allocated pages (cumulative)
  int total_free  = kpage_frees;               // total freed pages (cumulative)
  int current     = total_alloc - total_free;  // pages in use right now
  // frames managed by the page allocator: from end of kernel to PHYSTOP
  int total_frames = (PHYSTOP - V2P((uint)end)) / PGSIZE;
  int available    = kfree_pages();            // exact free-list length (kalloc.c)

  cprintf("memstats: total allocated pages = %d\n", total_alloc);
  cprintf("memstats: total freed pages = %d\n", total_free);
  cprintf("memstats: currently allocated pages = %d\n", current);
  cprintf("memstats: available physical memory pages = %d (of %d total frames)\n",
          available, total_frames);
  return 0;
}

// Assignment 10, Q2(a): syscall wrapper - arg is the virtual address.
int
sys_vtop(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return vtop((uint)va);
}

// Assignment 10, Q3(a): syscall wrapper - arg is the virtual address.
int
sys_pageinfo(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return pageinfo((uint)va);
}

// Assignment 10, Q3(b): syscall wrapper - arg is the virtual address.
int
sys_setreadonly(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return setreadonly((uint)va);
}

// ================= consolidated OS lab assignments 3-10 =================

// Set a field on the process with the given pid (ptable lookup).
// Used by the Assignment 3/4 syscalls, whose signatures the PDFs fix as
// set_burst(int pid, int bt) and set_priority(int pid, int priority).
static int
set_proc_field(int pid, int value, int is_priority)
{
  extern struct { struct spinlock lock; struct proc proc[NPROC]; } ptable;
  struct proc *p;
  int rc = -1;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->state != UNUSED){
      if(is_priority){
        p->priority = value;
      } else {
        p->burst_time = value;
        p->remaining_time = value; // A4 Q1 (SRTF) counts down from here
      }
      rc = 0;
      break;
    }
  }
  release(&ptable.lock);
  return rc;
}

// ---- Assignment 3 Q3 / Assignment 4: set_burst(int pid, int bt) ----
int
sys_set_burst(void)
{
  int pid, bt;
  if(argint(0, &pid) < 0 || argint(1, &bt) < 0 || bt <= 0)
    return -1;
  return set_proc_field(pid, bt, 0);
}

// ---- Assignment 3 Q2 / Assignment 7 Q4: set_priority(int pid, int pri)
// Priority range is fixed by the PDF: 1 = highest .. 20 = lowest. ----
int
sys_set_priority(void)
{
  int pid, pri;
  if(argint(0, &pid) < 0 || argint(1, &pri) < 0 || pri < 1 || pri > 20)
    return -1;
  return set_proc_field(pid, pri, 1);
}

int
sys_get_priority(void)
{
  return myproc()->priority;
}

// ---- Assignments 3-4: scheduler observability ----
// Copies the caller's scheduling metrics into a user struct pinfo.
int
sys_getpinfo(void)
{
  struct pinfo *pi;
  struct proc *p = myproc();
  if(argptr(0, (void*)&pi, sizeof(*pi)) < 0)
    return -1;
  pi->pid = p->pid;
  pi->state = p->state;
  pi->priority = p->priority;
  pi->queue_level = p->queue_level;
  pi->ctime = p->ctime;
  pi->rtime = p->rtime;
  pi->ready_since = p->ready_since;
  pi->burst_time = p->burst_time;
  pi->remaining = p->remaining_time;
  return 0;
}

// ---- Assignment 8 Q1: translate a user virtual address ----
int
sys_va2pa(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return va2pa((uint)va);   // vm.c: walks the caller's page table (verbose)
}

// ---- Assignment 8 Q2: physical memory statistics ----
int
sys_getmeminfo(void)
{
  struct meminfo *mi;
  struct proc *p = myproc();
  if(argptr(0, (void*)&mi, sizeof(*mi)) < 0)
    return -1;
  mi->total_pages = total_frames_count();          // kalloc.c
  mi->free_pages = kfree_pages();                  // kalloc.c (oslab10)
  mi->user_pages = p->sz / PGSIZE;
  mi->pt_pages = count_pt_pages(p->pgdir);         // vm.c
  return 0;
}

// ---- Assignment 9 Q1: dump the caller's page table ----
int
sys_vmprint(void)
{
  return vmprint(myproc()->pgdir);   // vm.c: dump + count of user pages
}

// ---- Assignment 9 Q2: lazy-allocation fault counter ----
int
sys_getfaults(void)
{
  return (int)myproc()->pgfaults;
}

// ---- Assignment 5: kernel shared page (see shm.c) ----
int
sys_shm_get(void)
{
  return shm_get();
}

// ---- Assignment 7 Q2 (+ Assignment 5): kernel semaphores (sem.c) ----
static int
sem_arg_id_value(int *id, int *value, int with_value)
{
  if(argint(0, id) < 0)
    return -1;
  if(with_value && argint(1, value) < 0)
    return -1;
  return 0;
}

int
sys_sem_init(void)
{
  int id, value;
  if(sem_arg_id_value(&id, &value, 1) < 0)
    return -1;
  return sem_init_id(id, value);
}

int
sys_sem_wait(void)
{
  int id;
  if(sem_arg_id_value(&id, 0, 0) < 0)
    return -1;
  return sem_wait_id(id);
}

int
sys_sem_signal(void)
{
  int id;
  if(sem_arg_id_value(&id, 0, 0) < 0)
    return -1;
  return sem_signal_id(id);
}

int
sys_sem_destroy(void)
{
  int id;
  if(sem_arg_id_value(&id, 0, 0) < 0)
    return -1;
  return sem_destroy_id(id);
}

// ---- Assignment 7 Q1: kernel threads (see proc.c) ----
// PDF signature: int clone(void(*fcn)(void*), void *arg, void *stack)
int
sys_clone(void)
{
  int fcn, arg, stack;
  if(argint(0, &fcn) < 0 || argint(1, &arg) < 0 || argint(2, &stack) < 0)
    return -1;
  return clone((void(*)(void*))fcn, (void*)arg, (void*)stack);
}

// PDF signature: int join(void) - waits for any thread of the group.
int
sys_join(void)
{
  return join();
}
