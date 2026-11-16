#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "sched.h" // consolidated lab build
#include "memlayout.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
#if LAZY_ALLOC
  case T_PGFLT:
    // Assignment 8 Q4 / Assignment 9 Q2: lazy allocation. sbrk() (see
    // sysproc.c) only records the new size; the first touch of each page
    // faults here and gets a frame on demand. This covers BOTH user-mode
    // touches and kernel-mode touches (a syscall copying to/from a lazy
    // user buffer) - the faulting address is what matters, not the mode.
    {
      struct proc *p = myproc();
      uint va;
      char *mem;
      if(p == 0)
        goto lazy_fail;
      va = rcr2();
      if(va >= p->sz || uvmpresent(p->pgdir, va))
        goto lazy_fail;              // outside the recorded heap: real fault
      mem = kalloc();
      if(mem == 0){
        cprintf("lazyalloc: out of memory\n");
        goto lazy_fail;
      }
      memset(mem, 0, PGSIZE);
      if(mappages(p->pgdir, (char*)PGROUNDDOWN(va), PGSIZE, V2P(mem),
                  PTE_W|PTE_U) < 0){
        kfree(mem);
        goto lazy_fail;
      }
      p->pgfaults++;                 // A9 Q2: counted for getfaults()
      break;
    lazy_fail:
      if((tf->cs&3) == 0)
        panic("lazyalloc: kernel page fault outside user heap");
      cprintf("pid %d %s: unhandled page fault at 0x%x\n",
              p ? p->pid : 0, p ? p->name : "?", rcr2());
      if(p)
        p->killed = 1;
    }
    break;
#endif
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  //
  // consolidated lab build: the SCHED policy (sched.h) owns the decision.
  // sched_charge() applies the policy's quantum accounting and returns
  // nonzero only for preemptive policies; FCFS/SJF/HRRN return 0, making
  // them non-preemptive as the assignments specify.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER){
    sched_tick();
    if(sched_charge(myproc()))
      yield();
  }

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
