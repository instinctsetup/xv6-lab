// Assignment 6, Question 1: Banker's algorithm (user-space simulation).
// Prints the safety check and a safe sequence for a classic 5-process,
// 3-resource instance, then demonstrates rejecting an unsafe request.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NP 5
#define NR 3

static int safe(int avail[NR], int max[NP][NR], int alloc[NP][NR],
                int seq[NP])
{
  int work[NR], finish[NP], need[NP][NR], n = 0;
  for(int j = 0; j < NR; j++) work[j] = avail[j];
  for(int i = 0; i < NP; i++){
    finish[i] = 0;
    for(int j = 0; j < NR; j++) need[i][j] = max[i][j] - alloc[i][j];
  }
  for(int count = 0; count < NP; count++){
    int found = -1;
    for(int i = 0; i < NP; i++){
      if(finish[i]) continue;
      int ok = 1;
      for(int j = 0; j < NR; j++)
        if(need[i][j] > work[j]){ ok = 0; break; }
      if(ok){ found = i; break; }
    }
    if(found < 0) return 0;              // unsafe
    for(int j = 0; j < NR; j++) work[j] += alloc[found][j];
    finish[found] = 1;
    seq[n++] = found;
  }
  return 1;
}

int
main(void)
{
  int avail[NR] = {3, 3, 2};
  int max[NP][NR] = {{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};
  int alloc[NP][NR] = {{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
  int seq[NP];

  printf(1, "bankers: checking classic 5x3 instance\n");
  if(safe(avail, max, alloc, seq)){
    printf(1, "bankers: state is SAFE, sequence: ");
    for(int i = 0; i < NP; i++) printf(1, "P%d%s", seq[i], i == NP-1 ? "\n" : " ");
  } else {
    printf(1, "bankers: state is UNSAFE (unexpected)\n");
  }

  // Pretend P1 requests (1,0,2): grant tentatively and re-check.
  int req[NR] = {1, 0, 2};
  for(int j = 0; j < NR; j++){ avail[j] -= req[j]; alloc[1][j] += req[j]; }
  printf(1, "bankers: P1 requests (1,0,2) -> %s\n",
         safe(avail, max, alloc, seq) ? "grant (still safe)"
                                      : "deny (would be unsafe)");
  printf(1, "bankers: test complete\n");
  exit();
}
