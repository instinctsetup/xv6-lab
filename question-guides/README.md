Question-by-question lab guides
===============================

One .txt per question: what it asks, how to build and run it, which files
to open, the expected output, what to show in the lab, and how the
solution is implemented. 29 questions across Assignments 3-10.


ASSIGNMENT 3
------------
  A3 Q1: FCFS scheduler
      file: A3-Q1-fcfs-scheduler.txt   |   xv6 test: fcfstest
  A3 Q2: Priority scheduler with aging
      file: A3-Q2-priority-scheduler-with-aging.txt   |   xv6 test: priotest
  A3 Q3: SJF scheduler with declared burst
      file: A3-Q3-sjf-scheduler-with-declared-burst.txt   |   xv6 test: sjftest

ASSIGNMENT 4
------------
  A4 Q1: SRTF scheduler (preemptive SJF)
      file: A4-Q1-srtf-scheduler-preemptive-sjf.txt   |   xv6 test: srtftest
  A4 Q2: HRRN scheduler
      file: A4-Q2-hrrn-scheduler.txt   |   xv6 test: hrrntest
  A4 Q3: MLFQ scheduler (3 queues)
      file: A4-Q3-mlfq-scheduler-3-queues.txt   |   xv6 test: mlfqtest

ASSIGNMENT 5
------------
  A5 Q1: Peterson's algorithm
      file: A5-Q1-peterson-s-algorithm.txt   |   xv6 test: peterson
  A5 Q2: Producer-consumer (bounded buffer)
      file: A5-Q2-producer-consumer-bounded-buffer.txt   |   xv6 test: prodcons
  A5 Q3: Readers-writers
      file: A5-Q3-readers-writers.txt   |   xv6 test: readwrite
  A5 Q4: Dining philosophers (deadlock-free)
      file: A5-Q4-dining-philosophers-deadlock-free.txt   |   xv6 test: dining

ASSIGNMENT 6
------------
  A6 Q1: Banker's algorithm
      file: A6-Q1-banker-s-algorithm.txt   |   xv6 test: bankers
  A6 Q2: Wait-for graph cycle detection
      file: A6-Q2-wait-for-graph-cycle-detection.txt   |   xv6 test: deadlockdetect
  A6 Q3: Bad vs fixed resource acquisition order
      file: A6-Q3-bad-vs-fixed-resource-acquisition-order.txt   |   xv6 test: resourceorder
  A6 Q4: Five processes, three bounded resource pools
      file: A6-Q4-five-processes-three-bounded-resource-pools.txt   |   xv6 test: syncdeadlock

ASSIGNMENT 7
------------
  A7 Q1: clone() and join(): kernel threads
      file: A7-Q1-clone-and-join-kernel-threads.txt   |   xv6 test: threadtest
  A7 Q2: Kernel semaphores fix the race
      file: A7-Q2-kernel-semaphores-fix-the-race.txt   |   xv6 test: synctest
  A7 Q3: Deadlock detection with live recovery
      file: A7-Q3-deadlock-detection-with-live-recovery.txt   |   xv6 test: deadlock
  A7 Q4: Priority scheduling with get_priority and aging
      file: A7-Q4-priority-scheduling-with-get-priority-and-aging.txt   |   xv6 test: priotest

ASSIGNMENT 8
------------
  A8 Q1: va2pa: virtual to physical translation
      file: A8-Q1-va2pa-virtual-to-physical-translation.txt   |   xv6 test: va2patest
  A8 Q2: getmeminfo: physical memory statistics
      file: A8-Q2-getmeminfo-physical-memory-statistics.txt   |   xv6 test: meminfo_test
  A8 Q3: vmprint: dump the page table
      file: A8-Q3-vmprint-dump-the-page-table.txt   |   xv6 test: vmprinttest
  A8 Q4: Lazy allocation (demand paging)
      file: A8-Q4-lazy-allocation-demand-paging.txt   |   xv6 test: lazytest

ASSIGNMENT 9
------------
  A9 Q1: Page table walk and count
      file: A9-Q1-page-table-walk-and-count.txt   |   xv6 test: pttest
  A9 Q2: Page fault handling with a fault counter
      file: A9-Q2-page-fault-handling-with-a-fault-counter.txt   |   xv6 test: faulttest
  A9 Q3: Page replacement: FIFO and LRU
      file: A9-Q3-page-replacement-fifo-and-lru.txt   |   xv6 test: pagerepl

ASSIGNMENT 10
-------------
  A10 Q1: memstats: allocation counters
      file: A10-Q1-memstats-allocation-counters.txt   |   xv6 test: memtest
  A10 Q2: vtop: translate an address
      file: A10-Q2-vtop-translate-an-address.txt   |   xv6 test: vttest
  A10 Q3: pageinfo and setreadonly
      file: A10-Q3-pageinfo-and-setreadonly.txt   |   xv6 test: pagetest
  A10 Q4: Page replacement: FIFO, LRU, Optimal
      file: A10-Q4-page-replacement-fifo-lru-optimal.txt   |   xv6 test: pagereplace
