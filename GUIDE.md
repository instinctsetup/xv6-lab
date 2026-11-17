# xv6-lab - one xv6 tree for all OS lab assignments 3-10 (practice)

A complete, ready-to-build xv6-public (x86) tree. Stock xv6 with every
proc field and system call the assignments need already built in, all 28
test programs included, default xv6 behavior preserved. Clone, `make`,
code in lab. Nothing else to assemble.

Default build = stock round-robin scheduler + eager allocation, exactly
like plain xv6. Scheduler policies and lazy allocation are compile-time
switches (below), so one tree serves every assignment.

## Build and run (modern Linux)

```sh
sudo apt-get install build-essential qemu-system-x86
make            # builds xv6.img and fs.img
make qemu CPUS=1      # or: make qemu-nox CPUS=1  (terminal-only)
```

The Makefile already carries `-Wno-array-bounds`, needed for GCC 12 and
newer (Ubuntu 22.10+/23/24): recent GCC raises a false-positive
array-bounds error in `mp.c` and `-Werror` makes it fatal. GCC 12+ also
flags the intentional recursion in `sh.c`'s `runcmd()` with
`-Winfinite-recursion`; the Makefile downgrades that one warning to
non-fatal for `sh.o` only (probed, so older GCC is unaffected) - the
warning still prints. With these flags, stock code builds unchanged. If
you see a *different* error, report it.

## Question-to-code map

| Assignment | Question | Where | Test |
|---|---|---|---|
| A3 | Q1 FCFS scheduler | `proc.c` (`key()`/`pick()`, `SCHED=FCFS`) | `fcfstest` |
| A3 | Q2 priority scheduler + aging | `proc.c` (`SCHED=PRIO`, `AGING_INTERVAL`) | `priotest` |
| A3 | Q3 SJF scheduler | `proc.c` (`SCHED=SJF`), `set_burst` | `sjftest` |
| A4 | Q1 SRTF scheduler | `proc.c`/`trap.c` (`SCHED=SRTF`) | `srtftest` |
| A4 | Q2 HRRN scheduler | `proc.c` (`SCHED=HRRN`) | `hrrntest` |
| A4 | Q3 MLFQ scheduler | `proc.c` (`SCHED=MLFQ`, 3 queues + boost) | `mlfqtest` |
| A5 | Q1 Peterson | `peterson.c` over `shm_get()` | `peterson` |
| A5 | Q2 producer-consumer | kernel semaphores `sem.c` | `prodcons` |
| A5 | Q3 readers-writers | kernel semaphores `sem.c` | `readwrite` |
| A5 | Q4 dining philosophers | kernel semaphores, resource ordering | `dining` |
| A6 | Q1 banker's algorithm | user-space simulation | `bankers` |
| A6 | Q2 deadlock detection | user-space wait-for simulation | `deadlockdetect` |
| A6 | Q3 prevention by ordering | kernel semaphores | `resourceorder` |
| A6 | Q4 sync deadlock demo | AB-BA deadlock + watchdog recovery | `syncdeadlock` |
| A7 | Q1 kernel threads | `proc.c` `clone()`/`join()` | `threadtest` |
| A7 | Q2 kernel semaphores | `sem.c` (`sem_init/wait/signal`, array of 10) | `synctest` |
| A7 | Q3 deadlock detect/recover | `proc->waiting_on` + `sem_destroy` | `deadlock` |
| A7 | Q4 priority scheduling | shared `SCHED=PRIO` implementation with A3 Q2 | `priotest` |
| A8 | Q1 va2pa | `vm.c` `va2pa()` (verbose PDE/PTE walk) | `va2patest` |
| A8 | Q2 memory statistics | `getmeminfo()`, `kalloc.c`/`vm.c` counters | `meminfo_test` |
| A8 | Q3 page table dump | `vm.c` `vmprint()` | `vmprinttest` |
| A8 | Q4 lazy allocation | `sys_sbrk` + `trap.c` `T_PGFLT` (`LAZY=1`) | `lazytest` |
| A9 | Q1 page table walk | shared `vmprint()` with A8 Q3 | `pttest` |
| A9 | Q2 lazy alloc + counter | shared lazy handler, `getfaults()` | `faulttest` |
| A9 | Q3 page replacement | user-space simulator | `pagerepl` |
| A10 | Q1 allocation counters | `kalloc.c` counters, `memstats` | `memtest` |
| A10 | Q2 vtop | `vm.c` `vtop()` | `vttest` |
| A10 | Q3 pageinfo + setreadonly | `vm.c` | `pagetest` |
| A10 | Q4 page replacement | user-space simulator | `pagereplace` |


## Build switches

### Scheduler selection (Assignments 3-4)

The scheduler questions are mutually exclusive replacements for xv6's
round-robin scheduler, so exactly one policy compiles in per build,
selected at build time (documented in `support/sched.h`):

    make SCHED=FCFS qemu     # A3 Q1    make SCHED=PRIO qemu  # A3 Q2 / A7 Q4
    make SCHED=SJF  qemu     # A3 Q3    make SCHED=SRTF qemu  # A4 Q1
    make SCHED=HRRN qemu     # A4 Q2    make SCHED=MLFQ qemu  # A4 Q3

The same macro drives the timer-interrupt behavior in `trap.c`
(preemptive policies yield on ticks, non-preemptive ones do not).

### Lazy allocation (A8 Q4 / A9 Q2)

    make LAZY=1 qemu
    $ lazytest     # O(1) sbrk, demand paging, kernel-side buffer access,
                   # fork with lazy pages - all verified
    $ faulttest    # getfaults() counts exactly the pages touched

The default build (`LAZY=0`) keeps eager allocation so stock tests
(`usertests`) behave exactly as before.


## Design decisions and simplifications (documented per assignment text)

- **One key/pick layer**: all six scheduling policies share a single
  scheduler loop in `proc.c`; only the ordering key, quantum accounting
  (`sched_charge`) and per-tick bookkeeping (`sched_tick`) differ.
- **Threads (A7 Q1)**: `clone()` shares the caller's page table instead of
  copying it. The address space is owned by the group leader; a leader
  exiting kills its threads; threads are reaped with `join()` (PDF
  signature `int join(void)`), never by `wait()`. `kill()` on any group
  member kills the group.
- **Shared memory (A5)**: stock xv6 has no shared memory across `fork()`,
  so `shm_get()` maps one kernel-owned frame at a fixed address just below
  KERNBASE (outside `[0, sz)` so `fork`/`exec`/`freevm` leave it alone).
  Parent and child each call `shm_get()` and see the same frame.
- **Semaphores (A7 Q2)**: id-based (`sem_init(int id, int value)`), fixed
  array of 10, built on `sleep()`/`wakeup()`. `sem_destroy()` (extension)
  wakes blocked waiters with an error so deadlock demos can recover.
- **Page replacement (A9 Q3, A10 Q4)** and **banker's/deadlock detection
  (A6 Q1/Q2)** are user-space simulations, as the assignment PDFs specify;
  they need no kernel changes.
- **Unset burst time**: per the A3 PDF, processes that never call
  `set_burst()` are treated as lowest priority by SJF/SRTF.


## Verification summary (qemu, CPUS=1)

- Scheduler order verified for FCFS, PRIO (incl. aging 18->17 while
  waiting), SJF (20/40/60), SRTF (short job preempts long), HRRN (long
  job's growing response ratio beats newly arrived shorts), MLFQ
  (CPU-bound process demoted queue 0 -> 2, short job stays at top).
- All synchronization tests report exact expected counters (Peterson
  20000, prodcons 100 in order, threadtest 300000).
- Lazy allocation: 32 MB `sbrk()` consumes 0 frames; touching 3 pages
  consumes exactly 3 frames; kernel syscalls on lazy buffers fault
  through transparently; out-of-bounds access still kills the process.
- Stock `usertests`: ALL TESTS PASSED on the default build.


## What "simplified" means here

Only documentation/paper-tooling files from stock xv6-public were removed
(BUGS, Notes, TRICKS, runoff*, pr.pl, printpcs, gdbutil, bochs config and
similar). Every kernel source file stays - xv6 needs all of them to boot.
All stock user programs (sh, ls, cat, usertests, ...) stay, plus the 28
assignment test programs. proc.h carries every assignment's fields
(commented per question); syscall.h/user.h/usys.S carry every syscall.

Per-question independent versions (one question per folder, for focused
study) live separately in instinctsetup/oslab-assignment-wise - this tree
does not replace them.
