// Assignment 4, Question 3 test: MLFQ (3 queues, quantum 1/2/4 ticks,
// boost every MLFQ_BOOST ticks).
// Build and run with: make qemu SCHED=MLFQ CPUS=1
// Expected: the CPU-bound child sinks to queue 2 (queue_level grows in
// getpinfo samples) while the short child finishes quickly at the top;
// the periodic boost lifts the CPU-bound child back to queue 0.
#include "types.h"
#include "stat.h"
#include "user.h"
#include "pinfo.h"

static void work(int me)
{
  volatile int i, x = 0;
  struct pinfo pi;
  for(i = 0; i < 300000000; i++){
    x += i;
    if(i % 100000000 == 0){
      getpinfo(&pi);
      printf(1, "mlfq: child %d at queue %d (rtime %d)\n",
             me, pi.queue_level, pi.rtime);
    }
  }
  printf(1, "mlfq: child %d done\n", me);
  exit();
}

int
main(void)
{
  if(fork() == 0) work(0);       // CPU-bound
  if(fork() == 0){ sleep(2); printf(1, "mlfq: short child done\n"); exit(); }
  wait(); wait();
  printf(1, "mlfq: test complete\n");
  exit();
}
