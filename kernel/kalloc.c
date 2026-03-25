// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run
{
    struct run *next;
};

struct
{
    struct spinlock lock;
    int use_lock;
    struct run *freelist;
} g_kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void kinit1(void *vstart, void *vend)
{
    initlock(&g_kmem.lock, "kmem");
    g_kmem.use_lock = 0x00;
    freerange(vstart, vend);
}

void kinit2(void *vstart, void *vend)
{
    freerange(vstart, vend);
    g_kmem.use_lock = 0x01;
}

void freerange(void *vstart, void *vend)
{
    char *p = (char *)PGROUNDUP((uint)vstart);

    for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
    {
        kfree(p);
    }
}
// PAGEBREAK: 21
//  Free the page of physical memory pointed at by v,
//  which normally should have been returned by a
//  call to kalloc().  (The exception is when
//  initializing the allocator; see kinit above.)
void kfree(char *v)
{
    struct run *r = nullptr;

    if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(v, 0x01, PGSIZE);

    if (g_kmem.use_lock)
        acquire(&g_kmem.lock);

    r = (struct run *)v;
    r->next = g_kmem.freelist;
    g_kmem.freelist = r;

    if (g_kmem.use_lock)
        release(&g_kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *kalloc(void)
{
    struct run *r = nullptr;

    if (g_kmem.use_lock)
        acquire(&g_kmem.lock);

    r = g_kmem.freelist;

    if (r)
        g_kmem.freelist = r->next;

    if (g_kmem.use_lock)
        release(&g_kmem.lock);

    return (char *)r;
}

int kfreecount()
{
    struct run *r = nullptr;
    int count = 0x00;

    if (g_kmem.use_lock)
        acquire(&g_kmem.lock);

    r = g_kmem.freelist;

    while (r)
    {
        count++;
        r = r->next;
    }

    if (g_kmem.use_lock)
        release(&g_kmem.lock);

    return count;
}

int ktotalbytes()
{
    return PHYSTOP;
}

int kfreebytes()
{
    return kfreecount() * PGSIZE;
}

int kusedbytes()
{
    return ktotalbytes() - kfreebytes();
}