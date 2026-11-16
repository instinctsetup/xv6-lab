// Assignment 4, Question 2 test: HRRN (highest response ratio next).
// Build and run with: make qemu SCHED=HRRN CPUS=1
//
// The parent hogs the (non-preemptive) CPU for ~30 ticks after creating
// the LONG job, so the long job accumulates waiting time. The short jobs
// are created only afterwards. Under SJF the shorts would run first; HRRN
// picks the long job because its response ratio (w+b)/b has grown past
// the shorts' - this is the anti-starvation property the question asks
// to demonstrate.
// Expected: the long job (burst 60) finishes FIRST, then the shorts.
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me, int burst)
{
  volatile int i, x = 0;
  sleep(25);
  for(i = 0; i < burst * 1000000; i++) x += i;
  struct pinfo pi;
  getpinfo(&pi);
  printf(1, "hrrn: child %d done (burst %d, waited %d)\n",
         me, burst, uptime() - pi.ctime - pi.rtime);
  exit();
}

int
main(void)
{
  volatile int i, x = 0;
  int p0, p1, p2;
  if((p0 = fork()) == 0) work(0, 60);      // long job waits from t~0
  set_burst(p0, 60);
  for(i = 0; i < 90000000; i++) x += i;    // parent hogs CPU ~30 ticks
  if((p1 = fork()) == 0) work(1, 10);      // shorts arrive late
  set_burst(p1, 10);
  if((p2 = fork()) == 0) work(2, 10);
  set_burst(p2, 10);
  wait(); wait(); wait();
  printf(1, "hrrn: test complete\n");
  exit();
}
