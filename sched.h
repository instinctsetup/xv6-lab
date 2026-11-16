// sched.h - Consolidated OS lab assignments 3-10 (practice build)
//
// Compile-time scheduler selection. The six policies below replace xv6's
// default round-robin scheduler; only ONE can be active per build because
// they are mutually exclusive answers to different assignment questions.
// Select with:  make SCHED=FCFS   (RR is the default)
//
//   RR   - stock xv6 round robin (default; no assignment)
//   FCFS - Assignment 3, Question 1: first come first served, non-preemptive
//   PRIO - Assignment 3, Question 2 / Assignment 7, Question 4:
//          priority-based preemptive scheduling with aging (shared impl)
//   SJF  - Assignment 3, Question 3: shortest job first, estimated burst
//   SRTF - Assignment 4, Question 1: shortest remaining time first, preemptive
//   HRRN - Assignment 4, Question 2: highest response ratio next
//   MLFQ - Assignment 4, Question 3: multilevel feedback queue
//
// The same macro also drives the timer-interrupt behaviour in trap.c
// (preemptive policies yield on ticks, non-preemptive ones do not) and the
// per-policy bookkeeping in proc.c.

#define SCHED_RR    0   // stock round robin
#define SCHED_FCFS  1   // A3 Q1
#define SCHED_PRIO  2   // A3 Q2 / A7 Q4 (single canonical implementation)
#define SCHED_SJF   3   // A3 Q3
#define SCHED_SRTF  4   // A4 Q1
#define SCHED_HRRN  5   // A4 Q2
#define SCHED_MLFQ  6   // A4 Q3

#ifndef SCHED
#define SCHED SCHED_RR
#endif

// Assignment 3 Q2 / Assignment 7 Q4: a RUNNABLE process that waits more
// than AGING_INTERVAL ticks gets its priority number decremented by 1.
#define AGING_INTERVAL 30

// Assignment 4 Q3 (MLFQ): three queues. Ticks a process may run at each
// level before demotion, and the periodic boost interval that moves every
// process back to the top queue so low-priority work cannot starve.
#define MLFQ_LEVELS   3
#define MLFQ_Q0_TICKS 1
#define MLFQ_Q1_TICKS 2
#define MLFQ_Q2_TICKS 4
#define MLFQ_BOOST    100

// Default estimated burst for processes that never call set_burst()
// (Assignments 3 Q3, 4 Q1, 4 Q2). Documented: treated as an average job.
#define DEFAULT_BURST 10

// Set to 1 to enable lazy allocation (Assignment 8 Q4 / Assignment 9 Q2):
// sbrk() then only records the new size and the T_PGFLT handler in trap.c
// allocates frames on first touch. Default build stays eager so stock
// tests (usertests) keep working.
#ifndef LAZY_ALLOC
#define LAZY_ALLOC 0
#endif
