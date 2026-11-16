// sem.c - Kernel counting semaphores.
//
// Assignment 7, Question 2: counting semaphores on top of xv6's
// sleep()/wakeup(), with the exact syscall surface the assignment asks for
// (sem_init / sem_wait / sem_signal over a fixed array of 10 semaphores).
// Assignment 5 (Questions 2-4) reuses the same semaphores for the
// producer-consumer, readers-writers and dining-philosophers programs.
//
// sem_destroy() is an extension beyond the assignment text: it frees a
// slot and wakes any waiters so a stuck program fails loudly instead of
// hanging forever.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "mmu.h"
#include "proc.h"

// ptable lives in proc.c; needed to index per-process hold counts.
extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

#define NSEM 10   // assignment 7 Q2(c): fixed-size array of semaphores

struct semaphore {
  int used;              // slot allocated by sem_init
  int value;             // current count
  struct spinlock lk;    // protects value; sleep() releases it while blocked
  int waiters;           // number of processes blocked on this semaphore
  int owner[NPROC];      // per-proc hold count (Assignment 7 Q3 deadlock aid)
};

static struct semaphore sems[NSEM];

int
sem_init_id(int id, int value)
{
  struct semaphore *s;

  if(id < 0 || id >= NSEM || value < 0)
    return -1;
  s = &sems[id];
  acquire(&s->lk);
  if(s->used){           // re-initialising a live semaphore is a caller bug
    release(&s->lk);
    return -1;
  }
  s->used = 1;
  s->value = value;
  s->waiters = 0;
  memset(s->owner, 0, sizeof(s->owner));
  release(&s->lk);
  return 0;
}

int
sem_wait_id(int id)
{
  struct semaphore *s;
  struct proc *p;

  if(id < 0 || id >= NSEM)
    return -1;
  s = &sems[id];
  p = myproc();
  acquire(&s->lk);
  if(!s->used){
    release(&s->lk);
    return -1;
  }
  // Block while the count is exhausted. sleep() releases s->lk while
  // blocked and re-acquires it before returning, so the value test stays
  // atomic. proc->killed breaks the loop if the process is killed.
  while(s->used && s->value == 0 && !(p && p->killed)){
    s->waiters++;
    if(p)
      p->waiting_on = id;   // Assignment 7 Q3: wait-for bookkeeping
    sleep(s, &s->lk);
    s->waiters--;
  }
  if(p)
    p->waiting_on = -1;
  if(!s->used){             // sem_destroy() woke us: fail, don't re-sleep
    release(&s->lk);
    return -1;
  }
  if(p && p->killed){
    release(&s->lk);
    return -1;
  }
  s->value--;
  if(p)
    s->owner[p - ptable.proc]++;
  release(&s->lk);
  return 0;
}

int
sem_signal_id(int id)
{
  struct semaphore *s;
  struct proc *p;

  if(id < 0 || id >= NSEM)
    return -1;
  s = &sems[id];
  p = myproc();
  acquire(&s->lk);
  if(!s->used){
    release(&s->lk);
    return -1;
  }
  if(p && s->owner[p - ptable.proc] > 0)
    s->owner[p - ptable.proc]--;
  s->value++;
  if(s->waiters > 0)
    wakeup(s);          // wakes every sleeper; they re-check value under lk
  release(&s->lk);
  return 0;
}

int
sem_destroy_id(int id)
{
  struct semaphore *s;

  if(id < 0 || id >= NSEM)
    return -1;
  s = &sems[id];
  acquire(&s->lk);
  if(!s->used){
    release(&s->lk);
    return -1;
  }
  s->used = 0;
  // Wake everyone blocked on the destroyed semaphore; they see used == 0
  // on re-check and fail instead of sleeping forever.
  wakeup(s);
  release(&s->lk);
  return 0;
}
