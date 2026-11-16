// Assignment 6, Question 2: deadlock detection (user-space simulation).
// Builds a wait-for graph from an allocation/request instance and runs
// the detection algorithm; reports the deadlocked process set.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NP 5
#define NR 3

int
main(void)
{
  // Instance crafted so P1<->P2 wait on each other (deadlocked) and
  // P0, P3, P4 can finish.
  int avail[NR] = {0, 0, 0};
  int alloc[NP][NR] = {{0,0,0},{1,0,0},{0,1,0},{0,0,1},{0,0,0}};
  int request[NP][NR] = {{0,0,0},{0,1,0},{1,0,0},{0,0,0},{0,0,0}};

  int work[NR], finish[NP];
  for(int j = 0; j < NR; j++) work[j] = avail[j];
  for(int i = 0; i < NP; i++){
    finish[i] = 0;
    int any = 0;
    for(int j = 0; j < NR; j++) if(alloc[i][j]) any = 1;
    if(!any) finish[i] = 1;      // holds nothing: cannot be deadlocked
  }
  for(int changed = 1; changed;){
    changed = 0;
    for(int i = 0; i < NP; i++){
      if(finish[i]) continue;
      int ok = 1;
      for(int j = 0; j < NR; j++)
        if(request[i][j] > work[j]){ ok = 0; break; }
      if(ok){
        for(int j = 0; j < NR; j++) work[j] += alloc[i][j];
        finish[i] = 1;
        changed = 1;
      }
    }
  }
  printf(1, "deadlockdetect: deadlocked processes:");
  int dead = 0;
  for(int i = 0; i < NP; i++)
    if(!finish[i]){ printf(1, " P%d", i); dead++; }
  printf(1, " (count %d, expected 2: P1 and P2) %s\n",
         dead, dead == 2 ? "PASS" : "FAIL");
  exit();
}
