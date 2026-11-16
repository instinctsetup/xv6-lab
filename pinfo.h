#ifndef _PINFO_H_
#define _PINFO_H_

// Scheduler observability for Assignments 3 and 4: per-process scheduling
// metrics returned by getpinfo(). Lets the test programs verify scheduler
// behaviour (completion order, waiting time, queue level) instead of
// guessing from interleaved prints.
struct pinfo {
  int pid;
  int state;        // xv6 enum procstate value
  int priority;     // current priority (1 high .. 20 low)
  int queue_level;  // MLFQ level
  uint ctime;       // creation time (ticks)
  uint rtime;       // total ticks run (charged by the scheduler)
  uint ready_since; // tick when the process last became RUNNABLE
  int burst_time;   // estimated burst (set_burst), -1 if never set
  int remaining;    // remaining burst (SRTF)
};

#endif
