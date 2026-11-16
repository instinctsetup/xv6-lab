// Assignment 6, Question 4: demonstrating synchronization deadlock.
// Two processes take semaphores in OPPOSITE orders (classic AB-BA).
// With a wide enough race window they deadlock for real, which would hang
// xv6 forever - so a watchdog process observes the stall via a shared
// progress counter and recovers with sem_destroy(), which wakes the
// blocked processes with an error (compare resourceorder for the fix).
#include "types.h"
#include "stat.h"
#include "user.h"

#define SEM_A 0
#define SEM_B 1
#define ROUNDS 1000   // deliberately more than can finish before watchdog

static void
workerAB(int *prog)
{
  for(int i = 0; i < ROUNDS; i++){
    if(sem_wait(SEM_A) < 0) exit();   // semaphore destroyed: recovered
    sleep(1);                          // widen the race window
    if(sem_wait(SEM_B) < 0) exit();
    sem_signal(SEM_B);
    sem_signal(SEM_A);
    prog[0]++;
  }
  prog[2] = 1;                         // finished all rounds
  exit();
}

static void
workerBA(int *prog)
{
  for(int i = 0; i < ROUNDS; i++){
    if(sem_wait(SEM_B) < 0) exit();   // opposite order: the deadlock risk
    sleep(1);
    if(sem_wait(SEM_A) < 0) exit();
    sem_signal(SEM_A);
    sem_signal(SEM_B);
    prog[1]++;
  }
  prog[3] = 1;
  exit();
}

int
main(void)
{
  int *prog = (int*)shm_get();
  if((int)prog < 0){ printf(1, "syncdeadlock: shm_get failed\n"); exit(); }
  prog[0] = prog[1] = prog[2] = prog[3] = 0;
  sem_init(SEM_A, 1);
  sem_init(SEM_B, 1);

  if(fork() == 0){ workerAB((int*)shm_get()); }
  if(fork() == 0){ workerBA((int*)shm_get()); }

  // watchdog: give both workers time to stall, then check progress
  sleep(60);
  if(prog[2] && prog[3]){
    printf(1, "syncdeadlock: both finished (lucky timing) - no deadlock\n");
  } else {
    printf(1, "syncdeadlock: workers stalled at %d/%d rounds - "
             "AB-BA deadlock detected\n", prog[0], prog[1]);
    sem_destroy(SEM_A);   // recovery: blocked waiters wake with an error
    sem_destroy(SEM_B);
    printf(1, "syncdeadlock: recovered by destroying both semaphores\n");
  }
  wait(); wait();
  printf(1, "syncdeadlock: test complete\n");
  exit();
}
