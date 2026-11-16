// Assignment 5, Question 4: dining philosophers, deadlock-free by
// resource ordering (philosopher 4 picks up the right fork first).
// Forks are semaphores 5..9, one per philosopher.
// Expected: every philosopher eats MEALS times; the run terminates.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NPHIL 5
#define MEALS 5
#define FORK_BASE 5

static void philosopher(int id)
{
  int left = FORK_BASE + id;
  int right = FORK_BASE + (id + 1) % NPHIL;
  for(int m = 0; m < MEALS; m++){
    // break the circular wait: the last philosopher reverses pickup order
    if(id == NPHIL - 1){
      sem_wait(right);
      sem_wait(left);
    } else {
      sem_wait(left);
      sem_wait(right);
    }
    printf(1, "dining: philosopher %d ate meal %d\n", id, m + 1);
    sem_signal(left);
    sem_signal(right);
  }
  exit();
}

int
main(void)
{
  for(int i = 0; i < NPHIL; i++)
    sem_init(FORK_BASE + i, 1);
  for(int i = 0; i < NPHIL; i++)
    if(fork() == 0)
      philosopher(i);
  for(int i = 0; i < NPHIL; i++)
    wait();
  printf(1, "dining: all philosophers finished - PASS\n");
  exit();
}
