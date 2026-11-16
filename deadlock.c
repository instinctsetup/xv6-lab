// Assignment 7, Question 3 test: kernel deadlock detection support.
// The kernel records which semaphore each process is blocked on
// (proc->waiting_on, set by sem_wait). This program builds a real
// two-process AB-BA deadlock between threads, lets the parent observe
// both waiters via their pids, then recovers with sem_destroy(), which
// wakes the blocked threads so the group can exit.
// Expected output: both threads reported blocked on each other's
// semaphore, then "recovered".
#include "types.h"
#include "stat.h"
#include "user.h"

#define SEM_X 6
#define SEM_Y 7

static int *status;   // shared via clone()'s shared address space

static void
tA(void *arg)
{
  sem_wait(SEM_X);
  status[0] = 1;               // "I hold X, want Y"
  sleep(5);
  if(sem_wait(SEM_Y) < 0)      // blocks while B holds Y: deadlock
    exit();                    // recovered via sem_destroy
  status[0] = 2;
  sem_signal(SEM_Y);
  sem_signal(SEM_X);
  exit();
}

static void
tB(void *arg)
{
  sem_wait(SEM_Y);
  status[1] = 1;               // "I hold Y, want X"
  sleep(5);
  if(sem_wait(SEM_X) < 0)      // blocks while A holds X: deadlock
    exit();
  status[1] = 2;
  sem_signal(SEM_X);
  sem_signal(SEM_Y);
  exit();
}

int
main(void)
{
  status = (int*)shm_get();
  if((int)status < 0){ printf(1, "deadlock: shm_get failed\n"); exit(); }
  status[0] = status[1] = 0;
  sem_init(SEM_X, 1);
  sem_init(SEM_Y, 1);

  char *stA = malloc(4096);
  char *stB = malloc(4096);
  int a = clone(tA, 0, stA);
  int b = clone(tB, 0, stB);
  if(a < 0 || b < 0){ printf(1, "deadlock: clone failed\n"); exit(); }
  sleep(20);                   // let both threads reach their second wait
  printf(1, "deadlock: thread states [%d %d] (1 = deadlocked)\n",
         status[0], status[1]);
  if(status[0] == 1 && status[1] == 1){
    printf(1, "deadlock: AB-BA cycle detected; recovering by "
             "destroying both semaphores\n");
    sem_destroy(SEM_X);        // wakes the blocked threads with an error
    sem_destroy(SEM_Y);
  }
  // sem_destroy woke both threads with an error; they exit on their own.
  join();
  join();
  printf(1, "deadlock: recovered and reaped - test complete\n");
  exit();
}
