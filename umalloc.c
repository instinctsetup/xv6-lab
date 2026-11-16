#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

// consolidated lab build (Assignment 7 Q1): kernel threads share one
// address space, so malloc/free need a user-space spinlock to stay
// correct. Simple test-and-set; no kernel call needed when uncontended.
static volatile int malock;

static void
m_lock(void)
{
  while(__sync_lock_test_and_set(&malock, 1))
    ;
}

static void
m_unlock(void)
{
  __sync_lock_release(&malock);
}

void
free(void *ap)
{
  Header *bp, *p;

  m_lock();
  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
  m_unlock();
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;
  Header *bp, *q;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  // consolidated build: inline free() WITHOUT taking malock, because
  // malloc() already holds it when calling morecore().
  bp = hp;   // block starts AT the header, like free(hp+1)
  for(q = freep; !(bp > q && bp < q->s.ptr); q = q->s.ptr)
    if(q >= q->s.ptr && (bp > q || bp < q->s.ptr))
      break;
  if(bp + bp->s.size == q->s.ptr){
    bp->s.size += q->s.ptr->s.size;
    bp->s.ptr = q->s.ptr->s.ptr;
  } else
    bp->s.ptr = q->s.ptr;
  if(q + q->s.size == bp){
    q->s.size += bp->s.size;
    q->s.ptr = bp->s.ptr;
  } else
    q->s.ptr = bp;
  freep = q;
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  m_lock();
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      m_unlock();
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0){
        m_unlock();
        return 0;
      }
  }
}
