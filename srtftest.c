// Assignment 4, Question 1 test: SRTF (shortest remaining time first).
// Build and run with: make qemu SCHED=SRTF CPUS=1
// Expected: the late-arriving short job preempts the long job and
// finishes before it, unlike non-preemptive SJF.
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me, int burst)
{
  volatile int i, x = 0;
  for(i = 0; i < burst * 1000000; i++) x += i;
  struct pinfo pi;
  getpinfo(&pi);
  printf(1, "srtf: child %d done (burst %d, remaining %d)\n",
         me, burst, pi.remaining);
  exit();
}

int
main(void)
{
  int p0, p1;
  set_burst(getpid(), 1);   // keep the parent schedulable while setting up
  if((p0 = fork()) == 0) work(0, 90);   // long job starts first
  set_burst(p0, 90);
  sleep(5);                             // let it run, then...
  if((p1 = fork()) == 0) work(1, 10);   // ...a short job arrives
  set_burst(p1, 10);
  wait(); wait();
  printf(1, "srtf: test complete\n");
  exit();
}
