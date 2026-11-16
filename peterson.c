// Assignment 5, Question 1: Peterson's algorithm for two processes.
// Uses shm_get() for the shared flag[2]/turn variables (stock xv6 has no
// shared memory across fork, so the kernel provides one shared frame).
// Expected: the shared counter ends at exactly 2 * ITER; any race shows
// up as a smaller total.
#include "types.h"
#include "stat.h"
#include "user.h"

#define ITER 10000

struct shared {
  int flag[2];
  int turn;
  int counter;
};

int
main(void)
{
  struct shared *sh;
  int addr = shm_get();
  if(addr < 0){ printf(1, "peterson: shm_get failed\n"); exit(); }
  sh = (struct shared*)addr;
  sh->flag[0] = sh->flag[1] = sh->turn = sh->counter = 0;

  int pid = fork();
  int me = (pid == 0) ? 1 : 0;
  int other = 1 - me;
  if(pid == 0){
    int caddr = shm_get();      // child maps the SAME frame
    if(caddr < 0){ printf(1, "peterson: child shm_get failed\n"); exit(); }
    sh = (struct shared*)caddr;
  }

  for(int i = 0; i < ITER; i++){
    sh->flag[me] = 1;
    sh->turn = other;
    while(sh->flag[other] && sh->turn == other)
      ;                          // spin
    sh->counter++;               // critical section
    sh->flag[me] = 0;
  }

  if(pid > 0){
    wait();
    printf(1, "peterson: counter=%d expected=%d %s\n", sh->counter,
           2 * ITER, sh->counter == 2 * ITER ? "PASS" : "FAIL");
  }
  exit();
}
