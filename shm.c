// shm.c - Assignment 5: one kernel-owned page shared between address spaces.
//
// The Assignment 5 questions (Peterson, producer-consumer, readers-writers,
// dining philosophers) need shared memory, which stock xv6 lacks because
// fork() copies the address space. shm_get() maps ONE kernel-owned frame
// into the caller's page table and returns its user virtual address.
// Parent and child each call shm_get() after fork() and thereby see the
// same physical frame.
//
// Simplifications, documented per the assignment's "document and justify"
// requirement:
//  - A single global page (4 KB) is enough for every test program.
//  - fork() copies the mapping as a private page too; the child must call
//    shm_get() itself and use that address, which every test program does.
//  - The frame is never freed (kernel-owned for the life of the boot).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "shm.h"

static struct spinlock shmlock;
static char *shm_frame = 0;    // kernel virtual address of the shared frame
static int shmlock_ready = 0;

int
shm_get(void)
{
  struct proc *p = myproc();
  uint va;

  if(p == 0)
    return -1;

  // Lazily allocate the shared frame exactly once.
  if(!shmlock_ready){
    initlock(&shmlock, "shm");
    shmlock_ready = 1;
  }
  acquire(&shmlock);
  if(shm_frame == 0){
    shm_frame = kalloc();
    if(shm_frame == 0){
      release(&shmlock);
      return -1;
    }
    memset(shm_frame, 0, PGSIZE);
  }
  release(&shmlock);

  // Map the frame at a FIXED virtual address just below KERNBASE,
  // deliberately OUTSIDE [0, sz): fork() does not copy it (the child calls
  // shm_get() itself and gets the same frame), and freevm() is taught in
  // vm.c to skip it, so the kernel-owned frame is never returned to the
  // allocator while still mapped - that aliasing bug corrupted memory.
  va = SHM_VA;
  if(uvmpresent(p->pgdir, va))
    return (int)va;     // already mapped in this address space
  if(mappages(p->pgdir, (char*)va, PGSIZE, V2P(shm_frame), PTE_W|PTE_U) < 0)
    return -1;
  return (int)va;
}
