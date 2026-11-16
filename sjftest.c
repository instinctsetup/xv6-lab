// Assignment 3, Question 3 test: SJF (shortest job first, estimated burst).
// Build and run with: make qemu SCHED=SJF CPUS=1
// The parent declares each child's burst by pid while the children sleep,
// so all three jobs are runnable with known bursts before any runs.
// Expected: completion order is short(20), medium(40), long(60) even
// though the long job was created first.
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me, int burst)
{
  volatile int i, x = 0;
  sleep(20);                     // let the parent declare all bursts first
  for(i = 0; i < burst * 1000000; i++) x += i;
  struct pinfo pi;
  getpinfo(&pi);
  printf(1, "sjf: child %d done (burst %d, rtime %d)\n", me, burst, pi.rtime);
  exit();
}

int
main(void)
{
  int p0, p1, p2;
  if((p0 = fork()) == 0) work(0, 60);   // long, created first
  set_burst(p0, 60);
  if((p1 = fork()) == 0) work(1, 20);   // short
  set_burst(p1, 20);
  if((p2 = fork()) == 0) work(2, 40);   // medium
  set_burst(p2, 40);
  wait(); wait(); wait();
  printf(1, "sjf: test complete\n");
  exit();
}
