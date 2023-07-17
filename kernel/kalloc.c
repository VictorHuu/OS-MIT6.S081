// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
struct{
  struct spinlock lock;
  int cnt[PHYSTOP/PGSIZE];
}ref;
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock,"ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref.cnt[(uint64)p/PGSIZE]=1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  acquire(&ref.lock);
  if(--ref.cnt[(uint64)pa/PGSIZE]==0){
	release(&ref.lock);
  	// Fill with junk to catch dangling refs.
  	memset(pa, 1, PGSIZE);

  	r = (struct run*)pa;

  	acquire(&kmem.lock);
  	r->next = kmem.freelist;
  	kmem.freelist = r;
  	release(&kmem.lock);
  }else
  	release(&ref.lock);
  
}
//Judge whether the very page is a cow page.
int cowpage(pagetable_t pagetable, uint64 va) {
  if(va >= MAXVA)
    return -1;
  pte_t* pte = walk(pagetable, va, 0);
  if(pte == 0)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  return (*pte & PTE_COW ? 0 : -1);
}

void* cowalloc(pagetable_t pagetable, uint64 va) {
  uint flags;
  if(va % PGSIZE != 0)
    return 0;

  uint64 pa = walkaddr(pagetable, va);  
  if(pa == 0)
    return 0;

  pte_t* pte = walk(pagetable, va, 0); 

  if(krefcnt((char*)pa) == 1) {//Only 1 reference
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
    return (void*)pa;
  } else {//late map a real page for the PTE
    char* mem = kalloc();
    if(mem == 0)
      return 0;

    memmove(mem, (char*)pa, PGSIZE);

    *pte &= ~PTE_V;//avoid remap error
    flags=PTE_FLAGS(*pte);
    
    flags=flags|PTE_W;
    flags=flags&~PTE_COW;
    if(mappages(pagetable, va, PGSIZE, (uint64)mem, flags) != 0) {
      kfree(mem);
      *pte |= PTE_V;//pair with the above PTE_V disabling.
      return 0;
    }


    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}
//Get the reference count of the physical address pa
int get_refcnt(void* pa) {
  return ref.cnt[(uint64)pa / PGSIZE];
}

//Increment the reference count of pa
int inc_refcnt(void* pa) {
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&ref.lock);
  ++ref.cnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  return 0;
}
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&ref.lock);
    ref.cnt[(uint64)r/PGSIZE]=1;
    release(&ref.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
