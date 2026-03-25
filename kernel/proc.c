#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct
{
    struct spinlock lock;
    struct proc proc[NPROC];
} g_ptable;

static struct proc *s_initproc = nullptr;

int g_nextpid = 0x01;

extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
    initlock(&g_ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
    return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *mycpu(void)
{
    int apicid = 0x00;

    if (readeflags() & FL_IF)
        panic("mycpu called with interrupts enabled\n");

    apicid = lapicid();
    // APIC IDs are not guaranteed to be contiguous. Maybe we should have
    // a reverse map, or reserve a register to store &cpus[i].
    for (int i = 0x00; i < ncpu; i++)
    {
        if (cpus[i].apicid == apicid)
            return &cpus[i];
    }

    panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *myproc(void)
{
    struct cpu  *c = nullptr;
    struct proc *p = nullptr;

    pushcli();
    c = mycpu();
    p = c->proc;
    popcli();

    return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *allocproc(void)
{
    struct proc *p = nullptr;

    char *sp = nullptr;

    acquire(&g_ptable.lock);
    for (p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->state == UNUSED)
            goto found;
    }
    release(&g_ptable.lock);
    return nullptr;

found:
    p->state          = EMBRYO;
    p->pid            = g_nextpid++;
    p->cpu_ticks      = 0x00;
    p->last_cpu_ticks = 0x00;

    release(&g_ptable.lock);

    // Allocate kernel stack.
    if ((p->kstack = kalloc()) == 0x00)
    {
        p->state = UNUSED;
        return nullptr;
    }

    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe *)sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 0x04;
    *(uint *)sp = (uint)trapret;

    sp -= sizeof *p->context;
    p->context      = (struct context *)sp;
    memset(p->context, 0x00, sizeof *p->context);
    p->context->eip = (uint)forkret;
    p->cloned       = false;
    p->detached     = false;
    p->ustack       = nullptr;

    return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void userinit(void)
{
    extern char _binary_initcode_start[];
    extern char _binary_initcode_size[];

    struct proc *p = allocproc();

    s_initproc = p;

    if ((p->pgdir = setupkvm()) == 0x00)
        panic("userinit: out of memory?");

    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    p->sz         = PGSIZE;
    memset(p->tf, 0x00, sizeof(*p->tf));
    p->tf->cs     = (SEG_UCODE << 0x03) | DPL_USER;
    p->tf->ds     = (SEG_UDATA << 0x03) | DPL_USER;
    p->tf->es     = p->tf->ds;
    p->tf->ss     = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp    = PGSIZE;
    p->tf->eip    = 0x00; // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&g_ptable.lock);
    p->state = RUNNABLE;
    release(&g_ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
    struct proc *curproc = myproc();

    uint sz = curproc->sz;

    if (n > 0x00)
    {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0x00)
            return -0x01;
    }
    else if (n < 0x00)
    {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0x00)
            return -0x01;
    }

    curproc->sz = sz;
    switchuvm(curproc);
    return 0x00;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
    struct proc *curproc = myproc();
    struct proc *np      = allocproc();

    int pid = -0x01;

    // Allocate process.
    if (np == nullptr)
        return -0x01;

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0x00)
    {
        kfree(np->kstack);
        np->kstack = 0x00;
        np->state  = UNUSED;
        return -0x01;
    }

    np->sz      = curproc->sz;
    np->parent  = curproc;
    *np->tf     = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0x00;

    for (int i = 0x00; i < NOFILE; i++)
    {
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    }

    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&g_ptable.lock);
    np->state = RUNNABLE;
    release(&g_ptable.lock);
    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
    struct proc *curproc = myproc();
    struct proc *p       = nullptr;

    int fd = -0x01;

    if (curproc == s_initproc)
        panic("init exiting");

    // Close all open files.
    for (fd = 0x00; fd < NOFILE; fd++)
    {
        if (curproc->ofile[fd])
        {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0x00;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0x00;

    acquire(&g_ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->parent == curproc)
        {
            p->parent = s_initproc;

            if (p->state == ZOMBIE)
                wakeup1(s_initproc);
        }
    }

    curproc->state = ZOMBIE;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
    struct proc *curproc = myproc();

    int pid = -0x01;

    acquire(&g_ptable.lock);

    while (true)
    {
        // Scan through table looking for exited children.
        bool havekids = false;

        for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
        {
            if (p->parent != curproc || p->cloned)
                continue;

            havekids = true;

            if (p->state == ZOMBIE)
            {
                // Found one.
                pid           = p->pid;
                kfree(p->kstack);
                p->kstack     = nullptr;

                if (p->cloned == false)
                    freevm(p->pgdir);

                p->pid        = 0x00;
                p->parent     = 0x00;
                p->name[0x00] = 0x00;
                p->killed     = 0x00;
                p->state      = UNUSED;
                release(&g_ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if (havekids == false || curproc->killed)
        {
            release(&g_ptable.lock);
            return -0x01;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &g_ptable.lock); // DOC: wait-sleep
    }
}

// PAGEBREAK: 42
//  Per-CPU process scheduler.
//  Each CPU calls scheduler() after setting itself up.
//  Scheduler never returns.  It loops, doing:
//   - choose a process to run
//   - swtch to start running that process
//   - eventually that process transfers control
//       via swtch back to the scheduler.
void scheduler(void)
{
    struct cpu *c = mycpu();

    c->proc = 0x00;

    for (;;)
    {
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&g_ptable.lock);
        for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
        {
            if (p->state == ZOMBIE && p->cloned && p->detached)
            {
                kfree(p->kstack);
                p->kstack     = nullptr;
                p->pid        = 0x00;
                p->parent     = 0x00;
                p->name[0x00] = 0x00;
                p->killed     = 0x00;
                p->state      = UNUSED;
                p->cloned     = false;
                p->detached   = false;
            }

            if (p->state != RUNNABLE)
                continue;

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            c->proc  = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc  = 0x00;
        }
        release(&g_ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
    struct proc *p = myproc();

    int intena = 0x00;

    if (!holding(&g_ptable.lock)) panic("sched ptable.lock");
    if (mycpu()->ncli != 0x01)    panic("sched locks");
    if (p->state == RUNNING)      panic("sched running");
    if (readeflags() & FL_IF)     panic("sched interruptible");

    intena = mycpu()->intena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    acquire(&g_ptable.lock); // DOC: yieldlock
    myproc()->state = RUNNABLE;
    sched();
    release(&g_ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
    static int first = 0x01;
    // Still holding ptable.lock from scheduler.
    release(&g_ptable.lock);

    if (first)
    {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0x00;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
    struct proc *p = myproc();

    if (p == 0x00)
        panic("sleep");

    if (lk == 0x00)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if (lk != &g_ptable.lock)
    {                          // DOC: sleeplock0
        acquire(&g_ptable.lock); // DOC: sleeplock1
        release(lk);
    }
    // Go to sleep.
    p->chan  = chan;
    p->state = SLEEPING;

    sched();

    // Tidy up.
    p->chan  = 0x00;

    // Reacquire original lock.
    if (lk != &g_ptable.lock)
    { // DOC: sleeplock2
        release(&g_ptable.lock);
        acquire(lk);
    }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void wakeup1(void *chan)
{
    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->state == SLEEPING && p->chan == chan)
            p->state = RUNNABLE;
    }
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
    acquire(&g_ptable.lock);
    wakeup1(chan);
    release(&g_ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
    acquire(&g_ptable.lock);
    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->pid == pid)
        {
            p->killed = 0x01;
            // Wake process from sleep if necessary.
            if (p->state == SLEEPING)
                p->state = RUNNABLE;

            release(&g_ptable.lock);
            return 0x00;
        }
    }
    release(&g_ptable.lock);
    return -0x01;
}

// PAGEBREAK: 36
//  Print a process listing to console.  For debugging.
//  Runs when user types ^P on console.
//  No lock to avoid wedging a stuck machine further.
void procdump(void)
{
    static char *states[] = 
    {
        [UNUSED]   "unused",
        [EMBRYO]   "embryo",
        [SLEEPING] "sleep ",
        [RUNNABLE] "runble",
        [RUNNING]  "run   ",
        [ZOMBIE]   "zombie"
    };

    uint pc[0x0A] = { 0x00 };

    char *state = nullptr;

    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->state == UNUSED)
            continue;

        state = (p->state >= 0x00 && p->state < NELEM(states) && states[p->state])
            ? states[p->state] 
            : "???";

        cprintf("%d %s %s", p->pid, state, p->name);

        if (p->state == SLEEPING)
        {
            getcallerpcs((uint *)p->context->ebp + 0x02, pc);

            for (int i = 0x00; i < 0x0A && pc[i] != 0x00; i++)
            {
                cprintf(" %p", pc[i]);
            }
        }

        cprintf("\n");
    }
}

int systicks(void)
{
    int total = 0x00;

    acquire(&g_ptable.lock);
    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->state == RUNNING)
            total += p->cpu_ticks;
    }
    release(&g_ptable.lock);
    return total;
}

int procticks(int pid)
{
    int ticks = 0x00;

    acquire(&g_ptable.lock);
    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->pid == pid)
        {
            ticks = p->cpu_ticks;
            break;
        }
    }
    release(&g_ptable.lock);
    return ticks;
}

int procs(struct uproc *up, int max)
{
    int i = 0x00;

    int total = systicks();

    acquire(&g_ptable.lock);
    for(struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if(p->state == UNUSED)
            continue;

        if(i >= max)
            break;

        up[i].pid       = p->pid;
        up[i].ppid      = p->parent ? p->parent->pid : 0x00;
        safestrcpy(up[i].name, p->name, sizeof(p->name));
        up[i].state     = p->state;

        int delta = p->cpu_ticks - p->last_cpu_ticks;
        p->last_cpu_ticks = p->cpu_ticks;

        up[i].cpu_usage = delta / 0x05;

        if (up[i].cpu_usage > 100)  up[i].cpu_usage = 100;
        if (up[i].cpu_usage < 0x00) up[i].cpu_usage = 0x00;

        // up[i].cpu_usage = total > 0x00 ? (p->cpu_ticks * 100) / total : 0x00;
        up[i].cpu_time  = p->cpu_ticks / 100;
        up[i].mem_usage = p->sz;

        i++;
    }
    release(&g_ptable.lock);
    return i;
}

int clone(void (*fn)(void*, void*), void *arg1, void *arg2, void *stack)
{
    struct proc *curproc = myproc();
    struct proc *np      = allocproc();

    int pid = -0x01;

    if (np == nullptr)
        return -0x01;

    np->pgdir  = curproc->pgdir;
    np->sz     = curproc->sz;
    np->parent = curproc;
    np->cloned = true;
    np->ustack = stack;

    *np->tf = *curproc->tf;

    if (stack == nullptr)
    {
        kfree(np->kstack);
        np->kstack = nullptr;

        acquire(&g_ptable.lock);
        np->state = UNUSED;
        release(&g_ptable.lock);

        return -0x01;
    }

    uint sp = (uint)stack + PGSIZE;

    sp -= 0x04;
    *(uint*)sp = (uint)arg2;
    sp -= 0x04;
    *(uint*)sp = (uint)arg1;
    sp -= 0x04;
    *(uint*)sp = 0xffffffff;

    np->tf->esp = sp;
    np->tf->eip = (uint)fn;

    for (int i = 0x00; i < NOFILE; i++)
    {
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    }

    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&g_ptable.lock);
    np->state = RUNNABLE;
    release(&g_ptable.lock);

    return pid;
}

int join(int pid)
{
    struct proc *curproc = myproc();

    acquire(&g_ptable.lock);

    while (true)
    {
        bool found = false;

        for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
        {
            if (p->pid == pid
                && p->parent == curproc
                && p->cloned
                && p->detached == false)
            {
                found = true;

                if (p->state == ZOMBIE)
                {
                    kfree(p->kstack);
                    p->kstack     = nullptr;
                    p->pid        = 0x00;
                    p->parent     = 0x00;
                    p->name[0x00] = 0x00;
                    p->killed     = 0x00;
                    p->state      = UNUSED;
                    p->cloned     = false;
                    p->detached   = false;
                    release(&g_ptable.lock);
                    return 0x00;
                }
            }
        }

        if (found == false)
        {
            release(&g_ptable.lock);
            return -0x01;
        }

        sleep(curproc, &g_ptable.lock);
    }
}

int detach(int pid)
{
    struct proc *curproc = myproc();

    acquire(&g_ptable.lock);

    for (struct proc *p = g_ptable.proc; p < &g_ptable.proc[NPROC]; p++)
    {
        if (p->pid == pid
            && p->parent == curproc
            && p->cloned)
        {
            p->detached = true;

            if (p->state == ZOMBIE)
            {
                kfree(p->kstack);
                p->kstack     = nullptr;
                p->pid        = 0x00;
                p->parent     = 0x00;
                p->name[0x00] = 0x00;
                p->killed     = 0x00;
                p->state      = UNUSED;
                p->cloned     = false;
                p->detached   = false;
            }

            release(&g_ptable.lock);
            return 0x00;
        }
    }

    release(&g_ptable.lock);
    return -0x01;
}