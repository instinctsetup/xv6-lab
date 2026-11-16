// Assignment 5, Question 3: readers-writers with kernel semaphores
// (reader-preference form). Semaphores: 3 = room (writer lock),
// 4 = mutex protecting readcount. Shared data in the shm_get() page.
// Expected: writers apply their updates one at a time; final data value
// equals the number of writer updates, and no reader ever observes the
// intermediate "torn" value.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NWRITERS 2
#define WITER 20
#define SEM_ROOM  3
#define SEM_RMUTEX 4

struct shared { int data; int torn; int readcount; };

int
main(void)
{
  struct shared *sh = (struct shared*)shm_get();
  if((int)sh < 0){ printf(1, "readwrite: shm_get failed\n"); exit(); }
  sh->data = 0; sh->torn = 0; sh->readcount = 0;
  sem_init(SEM_ROOM, 1);
  sem_init(SEM_RMUTEX, 1);

  for(int w = 0; w < NWRITERS; w++){
    if(fork() == 0){
      sh = (struct shared*)shm_get();
      for(int i = 0; i < WITER; i++){
        sem_wait(SEM_ROOM);
        sh->data++;              // critical section
        sem_signal(SEM_ROOM);
      }
      exit();
    }
  }
  for(int r = 0; r < 2; r++){
    if(fork() == 0){
      sh = (struct shared*)shm_get();
      for(int i = 0; i < 200; i++){
        sem_wait(SEM_RMUTEX);
        sh->readcount++;
        if(sh->readcount == 1) sem_wait(SEM_ROOM);
        sem_signal(SEM_RMUTEX);
        if(sh->data < 0 || sh->data > NWRITERS * WITER) sh->torn = 1;
        sem_wait(SEM_RMUTEX);
        sh->readcount--;
        if(sh->readcount == 0) sem_signal(SEM_ROOM);
        sem_signal(SEM_RMUTEX);
      }
      exit();
    }
  }
  for(int i = 0; i < NWRITERS + 2; i++) wait();
  printf(1, "readwrite: data=%d expected=%d torn=%d %s\n", sh->data,
         NWRITERS * WITER, sh->torn,
         (sh->data == NWRITERS * WITER && !sh->torn) ? "PASS" : "FAIL");
  exit();
}
