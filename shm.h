#ifndef _SHM_H_
#define _SHM_H_

// Assignment 5: fixed user virtual address of the single kernel-owned
// shared page (see shm.c). Chosen just below KERNBASE so it never
// collides with heap, stack or lazy-allocation accounting, and outside
// [0, sz) so fork() and exec() leave it alone.
#define SHM_VA (KERNBASE - PGSIZE)

#endif
