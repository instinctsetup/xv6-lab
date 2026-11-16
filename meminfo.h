#ifndef _MEMINFO_H_
#define _MEMINFO_H_

// Assignment 8, Question 2: structure filled by the getmeminfo() syscall
// with physical-memory statistics.
struct meminfo {
  int total_pages; // physical frames managed by the kernel allocator
  int free_pages;  // currently available free physical frames
  int user_pages;  // pages in the caller's user address space (sz / PGSIZE)
  int pt_pages;    // pages used by the caller's page directory + page tables
};

#endif
