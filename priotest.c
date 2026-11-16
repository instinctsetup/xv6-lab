// Assignment 3, Question 2 / Assignment 7, Question 4 test: preemptive
// priority scheduling with aging (single implementation, two labels).
// Build and run with: make qemu SCHED=PRIO CPUS=1
// Expected: the high-priority (low number) child finishes first even
// though it was created last; the starved child still completes thanks
// to aging (its priority improves after AGING_INTERVAL ticks waiting).
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me)
{
  volatile int i, x = 0;
  for(i = 0; i < 80000000; i++) x += i;
  struct pinfo pi;
  getpinfo(&pi);
  printf(1, "prio: child %d done (pid %d, priority %d)\n",
         me, pi.pid, pi.priority);
  exit();
}

int
main(void)
{
  set_burst(getpid(), 80);
  if(fork() == 0){ set_priority(getpid(), 18); work(0); }   // low priority, created 1st
  if(fork() == 0){ set_priority(getpid(), 10); work(1); }   // mid priority
  if(fork() == 0){ set_priority(getpid(), 2);  work(2); }   // high priority, created last
  wait(); wait(); wait();
  printf(1, "prio: test complete\n");
  exit();
}
