// Assignment 5, Question 2: producer-consumer with kernel semaphores.
// One producer, one consumer, bounded buffer in the shm_get() page.
// Semaphores: 0 = empty slots (N), 1 = full slots (0), 2 = mutex (1).
// Expected: consumer sees every produced value exactly once, in order.
#include "types.h"
#include "stat.h"
#include "user.h"

#define N 8
#define ITEMS 100
#define SEM_EMPTY 0
#define SEM_FULL  1
#define SEM_MUTEX 2

struct shared { int buf[N]; int in; int out; };

int
main(void)
{
  struct shared *sh = (struct shared*)shm_get();
  if((int)sh < 0){ printf(1, "prodcons: shm_get failed\n"); exit(); }
  sh->in = sh->out = 0;
  sem_init(SEM_EMPTY, N);
  sem_init(SEM_FULL, 0);
  sem_init(SEM_MUTEX, 1);

  int pid = fork();
  if(pid == 0){
    sh = (struct shared*)shm_get();   // same frame in the child
    for(int i = 1; i <= ITEMS; i++){  // producer
      sem_wait(SEM_EMPTY);
      sem_wait(SEM_MUTEX);
      sh->buf[sh->in] = i;
      sh->in = (sh->in + 1) % N;
      sem_signal(SEM_MUTEX);
      sem_signal(SEM_FULL);
    }
    exit();
  }
  // consumer (parent)
  int ok = 1;
  for(int i = 1; i <= ITEMS; i++){
    sem_wait(SEM_FULL);
    sem_wait(SEM_MUTEX);
    int v = sh->buf[sh->out];
    sh->out = (sh->out + 1) % N;
    sem_signal(SEM_MUTEX);
    sem_signal(SEM_EMPTY);
    if(v != i) ok = 0;
  }
  wait();
  printf(1, "prodcons: %d items %s\n", ITEMS, ok ? "PASS" : "FAIL");
  exit();
}
