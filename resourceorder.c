// Assignment 6, Question 3: deadlock prevention by resource ordering.
// Two processes each need semaphores 0 and 1; both acquire them in
// increasing id order, so no circular wait can form. Compare with the
// syncdeadlock program, which takes them in opposite orders.
// Expected: both processes finish; PASS.
#include "types.h"
#include "stat.h"
#include "user.h"

#define SEM_A 0
#define SEM_B 1
#define ITER 20

static void worker(int me)
{
  for(int i = 0; i < ITER; i++){
    // canonical order: lower semaphore id first, always
    sem_wait(SEM_A);
    sem_wait(SEM_B);
    sem_signal(SEM_B);           // release in reverse order
    sem_signal(SEM_A);
  }
  printf(1, "resourceorder: worker %d finished %d rounds\n", me, ITER);
  exit();
}

int
main(void)
{
  sem_init(SEM_A, 1);
  sem_init(SEM_B, 1);
  if(fork() == 0) worker(0);
  if(fork() == 0) worker(1);
  wait(); wait();
  printf(1, "resourceorder: no deadlock - PASS\n");
  exit();
}
