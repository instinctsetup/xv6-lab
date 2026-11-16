// Assignment 3, Question 1 test: FCFS (first come first served).
// Build and run with: make qemu SCHED=FCFS CPUS=1
// Expected: children finish strictly in creation order (c0, c1, c2),
// even though each gets the same CPU work.
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me)
{
  volatile int i, x = 0;
  for(i = 0; i < 50000000; i++) x += i;
  struct pinfo pi;
  getpinfo(&pi);
  printf(1, "fcfs: child %d done (pid %d, rtime %d)\n", me, pi.pid, pi.rtime);
  exit();
}

int
main(void)
{
  int i;
  set_burst(getpid(), 50);
  for(i = 0; i < 3; i++)
    if(fork() == 0)
      work(i);
  for(i = 0; i < 3; i++)
    wait();
  printf(1, "fcfs: test complete\n");
  exit();
}
