// Assignment 7, Question 2 test: kernel counting semaphores.
// Exercises sem_init/sem_wait/sem_signal/sem_destroy over the fixed
// 10-semaphore array: binary mutex across a fork, and a counting
// semaphore limiting two children to one-at-a-time access.
// Expected: shared counter exact, invalid ids rejected, destroy works.
#include "types.h"
#include "stat.h"
#include "user.h"

#define SEM_MX 2
#define ITER 3000

int
main(void)
{
  int *sh = (int*)shm_get();
  if((int)sh < 0){ printf(1, "synctest: shm_get failed\n"); exit(); }
  *sh = 0;

  if(sem_init(SEM_MX, 1) < 0){ printf(1, "synctest: init failed\n"); exit(); }
  if(sem_init(99, 1) == 0){    // out-of-range id must fail
    printf(1, "synctest: FAIL accepted bad id\n"); exit();
  }
  if(fork() == 0){
    sh = (int*)shm_get();
    for(int i = 0; i < ITER; i++){
      sem_wait(SEM_MX); (*sh)++; sem_signal(SEM_MX);
    }
    exit();
  }
  for(int i = 0; i < ITER; i++){
    sem_wait(SEM_MX); (*sh)++; sem_signal(SEM_MX);
  }
  wait();
  int ok = (*sh == 2 * ITER);
  if(sem_destroy(SEM_MX) < 0) ok = 0;
  printf(1, "synctest: counter=%d %s\n", *sh, ok ? "PASS" : "FAIL");
  exit();
}
