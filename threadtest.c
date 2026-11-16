// Assignment 7, Question 1(e) test: clone()/join() kernel threads.
// PDF spec: create 3 threads with clone(), each incrementing a shared
// global counter 100,000 times, then join() each and print the counter.
// Threads share the address space, so the global counter is genuinely
// shared; a kernel semaphore (id 0) serializes the increments.
// Expected: final counter = 300000 -> PASS.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREADS 3
#define ITER 100000
#define SEM_MX 0

static int counter = 0;

static void
worker(void *arg)
{
  int me = *(int*)arg;
  for(int i = 0; i < ITER; i++){
    sem_wait(SEM_MX);
    counter++;
    sem_signal(SEM_MX);
  }
  printf(1, "threadtest: thread %d done\n", me);
  exit();
}

int
main(void)
{
  int ids[NTHREADS] = {0, 1, 2};
  sem_init(SEM_MX, 1);
  for(int i = 0; i < NTHREADS; i++){
    char *st = malloc(4096);
    if(!st){ printf(1, "threadtest: malloc failed\n"); exit(); }
    if(clone(worker, &ids[i], st) < 0){
      printf(1, "threadtest: clone %d failed\n", i);
      exit();
    }
  }
  for(int i = 0; i < NTHREADS; i++)
    printf(1, "threadtest: joined tid %d\n", join());
  printf(1, "threadtest: counter=%d expected=%d %s\n", counter,
         NTHREADS * ITER, counter == NTHREADS * ITER ? "PASS" : "FAIL");
  exit();
}
