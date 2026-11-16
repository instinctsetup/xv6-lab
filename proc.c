#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "sched.h" // consolidated lab build: compile-time scheduler selection
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  // ---- consolidated lab build: per-process defaults ----
  p->ctime = ticks;                    // A3 Q1/A4 Q2
  p->burst_time = -1;                  // A3 Q3/A4: unset until set_burst()
  p->remaining_time = 0;               // A4 Q1 (SRTF)
  p->priority = 10;                    // A3 Q2/A7 Q4: mid-range default
  p->ready_since = ticks;
  p->last_run = 0;
  p->rtime = 0;
  p->queue_level = 0;                  // A4 Q3: new work enters the top queue
  p->qticks = 0;
  p->is_thread = 0;                    // A7 Q1
  p->leader = p;                       // a normal process leads its own group
  p->nthreads = 0;
  p->waiting_on = -1;                  // A7 Q3
  p->pgfaults = 0;                     // A9 Q2

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  // Threads share this pgdir: keep their cached sz in sync so a lazy
  // fault taken by a sibling thread sees the grown range as valid.
  { struct proc *q;
    acquire(&ptable.lock);
    for(q = ptable.proc; q < &ptable.proc[NPROC]; q++)
      if(q != curproc && q->state != UNUSED && q->pgdir == curproc->pgdir)
        q->sz = sz;
    release(&ptable.lock);
  }
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // consolidated lab build: children inherit the parent's scheduler
  // parameters (Assignments 3-4).
  np->priority = curproc->priority;
  np->burst_time = curproc->burst_time;
  np->remaining_time = curproc->remaining_time;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // ---- consolidated lab build (A7 Q1): thread-aware teardown ----
  if(curproc->is_thread){
    // A thread does not own the address space: skip parent->init reparent
    // games below and let join() reap it. Wake any join() sleeper.
    if(curproc->leader && curproc->leader != curproc)
      wakeup1(curproc->leader);
    curproc->state = ZOMBIE;
    sched();
    panic("zombie thread exit");
  }
  // A group leader exiting kills its threads: they share its address
  // space, which is about to be freed.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->is_thread && p->leader == curproc){
      p->killed = 1;
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
    }
  }

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      if(p->is_thread)
        continue;   // consolidated lab build: threads are joined, not waited
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
// ---- consolidated lab build (Assignments 3-4): policy layer ----
//
// One key() computes how "deserving" a process is under the selected
// SCHED policy (lower key wins), and pick() scans the table once for the
// best RUNNABLE process. Ties always break toward the process that ran
// least recently so no process is starved by another with an identical
// key. The scheduler() loop below is then policy-independent; the policy
// only changes key()/pick() and the timer behaviour in trap.c.

// Aging (A3 Q2 / A7 Q4): a process that has waited more than
// AGING_INTERVAL ticks in RUNNABLE gets one priority level better.
static int
effective_priority(struct proc *p)
{
  int pr = p->priority;
  if(pr > 1 && ticks - p->ready_since > AGING_INTERVAL)
    pr--;
  return pr;
}

static uint
key(struct proc *p)
{
  switch(SCHED){
  case SCHED_FCFS:                    // A3 Q1: earliest creation wins
    return p->ctime;
  case SCHED_PRIO:                    // A3 Q2 / A7 Q4: lowest number wins
    return (uint)effective_priority(p);
  case SCHED_SJF:                     // A3 Q3: shortest estimated burst;
    // PDF: processes with no burst set are treated as lowest priority.
    return (uint)(p->burst_time < 0 ? 0x7fffffff : p->burst_time);
  case SCHED_SRTF:                    // A4 Q1: shortest remaining time;
    // unset counts as the default burst, never ahead of a known short job.
    if(p->burst_time < 0)
      return 0x7fffffff;
    return (uint)(p->remaining_time >= 0 ? p->remaining_time : p->burst_time);
  case SCHED_HRRN: {                  // A4 Q2: highest response ratio wins,
    // so we return the inverse as a "lowest wins" key. Response ratio
    // rr = (waiting + burst) / burst; rr >= 1, so 1000000/rr orders the
    // largest rr first. Waiting time uses ticks since the process last
    // became RUNNABLE.
    int b = p->burst_time < 0 ? DEFAULT_BURST : p->burst_time;
    uint w = ticks - p->ready_since;
    uint rr = w + (uint)b;            // (w+b)/b with b folded out below
    if(b <= 0) b = 1;
    return 1000000U / (rr / (uint)b + 1);
  }
  case SCHED_MLFQ:                    // A4 Q3: top queue first
  case SCHED_RR:
  default:                            // stock round robin
    return 0;
  }
}

static struct proc*
pick(void)
{
  struct proc *p, *best = 0;
  uint bkey = 0;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    uint k;
    if(p->state != RUNNABLE)
      continue;
    if(SCHED == SCHED_MLFQ){
      // A4 Q3: never pick a lower queue while a higher queue has work.
      if(best != 0){
        if(p->queue_level > best->queue_level)
          continue;
        if(p->queue_level == best->queue_level &&
           p->last_run >= best->last_run)
          continue;
        best = p;
      } else {
        best = p;
      }
      continue;
    }
    k = key(p);
    if(best == 0 || k < bkey || (k == bkey && p->last_run < best->last_run)){
      best = p;
      bkey = k;
    }
  }
  return best;
}

// Periodic bookkeeping for the preemptive dynamic policies.
// Called from trap.c on timer ticks with ptable.lock NOT held, so we take
// it here. Walking the table once per tick is O(NPROC) and fine for xv6.
void
sched_tick(void)
{
  struct proc *p;

  if(SCHED != SCHED_PRIO && SCHED != SCHED_MLFQ)
    return;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(SCHED == SCHED_PRIO){
      // A3 Q2/A7 Q4 aging: persist the effective-priority improvement so
      // long-waiting processes ratchet upward level by level.
      if(p->state == RUNNABLE && p->priority > 1 &&
         ticks - p->ready_since > AGING_INTERVAL){
        p->priority--;
        p->ready_since = ticks;
      }
    } else {
      // A4 Q3 MLFQ: periodic priority boost prevents starvation.
      if(ticks > 0 && ticks % MLFQ_BOOST == 0){
        p->queue_level = 0;
        p->qticks = 0;
      }
    }
  }
  release(&ptable.lock);
}

// Charge one tick of runtime and apply per-policy quantum accounting.
// Called from trap.c while handling a timer interrupt on a RUNNING proc.
// Returns 1 when the policy wants the CPU back (preemption).
int
sched_charge(struct proc *p)
{
  p->rtime++;
  p->qticks++;
  switch(SCHED){
  case SCHED_SRTF:                     // A4 Q1
    if(p->burst_time >= 0 && p->remaining_time > 0)
      p->remaining_time--;
    return 1;                          // re-pick every tick
  case SCHED_MLFQ: {                   // A4 Q3: demote at quantum expiry
    int limit = p->queue_level == 0 ? MLFQ_Q0_TICKS :
                p->queue_level == 1 ? MLFQ_Q1_TICKS : MLFQ_Q2_TICKS;
    if(p->qticks >= limit){
      if(p->queue_level < MLFQ_LEVELS - 1)
        p->queue_level++;
      p->qticks = 0;
    }
    return 1;
  }
  case SCHED_RR:
  case SCHED_PRIO:                     // A3 Q2/A7 Q4: preemptive, quantum 1
    return 1;
  case SCHED_FCFS:                     // A3 Q1: non-preemptive
  case SCHED_SJF:                      // A3 Q3: non-preemptive
  case SCHED_HRRN:                     // A4 Q2: non-preemptive
  default:
    return 0;
  }
}

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Pick the most deserving RUNNABLE process under the SCHED policy.
    acquire(&ptable.lock);
    p = pick();
    if(p != 0){
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->last_run = ticks;      // consolidated build: RR tie-break

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  myproc()->ready_since = ticks;   // consolidated build: aging/HRRN window
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// ---- Assignment 7, Question 1: kernel threads ----
//
// clone() creates a thread that SHARES the caller's address space (page
// directory) instead of copying it like fork(). The caller supplies a
// user-space stack that belongs to that shared address space, so the new
// thread can run the function immediately. join() blocks until a specific
// thread of the caller's group exits.
//
// Resource ownership (documented per the assignment): the address space
// is owned by the group leader; threads do not free it in exit(). The
// leader exiting kills the whole group (exit() below), mirroring how
// POSIX threads die with their process.

int
clone(void (*fcn)(void*), void *arg, void *stack)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  struct proc *leader = curproc->is_thread ? curproc->leader : curproc;

  if(fcn == 0 || stack == 0)
    return -1;
  // The kernel writes the fake frame on the user stack below; reject a
  // stack outside the address space instead of faulting in kernel mode.
  if((uint)stack + PGSIZE > curproc->sz)
    return -1;

  if((np = allocproc()) == 0)
    return -1;

  // Share the caller's page table instead of copyuvm().
  np->pgdir = curproc->pgdir;
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Inherit scheduler parameters like fork().
  np->priority = curproc->priority;
  np->burst_time = curproc->burst_time;
  np->remaining_time = curproc->remaining_time;

  // Start the thread at fcn(arg) on the supplied user stack.
  // Layout (x86 cdecl): [ret][arg] with ret = 0; a thread that returns
  // without calling exit() faults at address 0 and is killed loudly
  // rather than corrupting anything.
  np->tf->eip = (uint)fcn;
  np->tf->esp = (uint)stack + PGSIZE - 8;
  *((uint*)(np->tf->esp))     = 0;              // fake return address
  *((uint*)(np->tf->esp + 4)) = (uint)arg;
  np->tf->ebp = np->tf->esp;

  // Threads share open files and cwd by reference.
  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  np->is_thread = 1;
  np->leader = leader;
  pid = np->pid;

  acquire(&ptable.lock);
  leader->nthreads++;
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

// Wait for ANY thread of the caller's group to exit and reap it;
// returns the reaped thread's pid. PDF signature: int join(void).
int
join(void)
{
  struct proc *p;
  struct proc *curproc = myproc();
  struct proc *leader = curproc->is_thread ? curproc->leader : curproc;

  acquire(&ptable.lock);
  for(;;){
    int found = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(!p->is_thread || p->leader != leader || p->state == UNUSED)
        continue;
      found = 1;
      if(p->state == ZOMBIE){
        int tid = p->pid;
        kfree(p->kstack);        // kernel stack only; the address space
        p->kstack = 0;           // and the user stack belong to the leader
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->is_thread = 0;
        p->leader = p;
        leader->nthreads--;
        release(&ptable.lock);
        return tid;
      }
    }
    if(!found || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(leader, &ptable.lock);   // exit() wakeup1(leader) covers any joiner
  }
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      // consolidated build: fresh waiting-time window for aging/HRRN.
      p->ready_since = ticks;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      struct proc *q;
      p->killed = 1;
      // consolidated lab build (A7 Q1): killing a group member kills the
      // whole group; the address space cannot outlive its leader.
      {
        struct proc *leader = p->is_thread ? p->leader : p;
        for(q = ptable.proc; q < &ptable.proc[NPROC]; q++){
          if(q != p && (q == leader || (q->is_thread && q->leader == leader))){
            q->killed = 1;
            if(q->state == SLEEPING)
              q->state = RUNNABLE;
          }
        }
      }
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
