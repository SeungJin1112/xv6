#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int fetchint(uint addr, int *ip)
{
    struct proc *curproc = myproc();

    if (addr >= curproc->sz || addr + 0x04 > curproc->sz)
        return -0x01;

    *ip = *(int *)(addr);

    return 0x00;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int fetchstr(uint addr, char **pp)
{
    struct proc *curproc = myproc();

    char *s  = nullptr;
    char *ep = nullptr;

    if (addr >= curproc->sz)
        return -0x01;

    *pp = (char *)addr;
    ep  = (char *)curproc->sz;

    for (s = *pp; s < ep; s++)
    {
        if (*s == 0x00)
            return s - *pp;
    }

    return -0x01;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip)
{
    return fetchint((myproc()->tf->esp) + 0x04 + 0x04 * n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int argptr(int n, char **pp, int size)
{
    struct proc *curproc = myproc();

    int i = 0x00;

    if (argint(n, &i) < 0x00)
        return -0x01;

    if (size < 0x00 || (uint)i >= curproc->sz || (uint)i + size > curproc->sz)
        return -0x01;

    *pp = (char *)i;

    return 0x00;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int argstr(int n, char **pp)
{
    int addr = -0x01;

    if (argint(n, &addr) < 0x00)
        return -0x01;

    return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_consolecolor(void);
extern int sys_consoleputsxy(void);
extern int sys_consolebox(void);
extern int sys_consolewindow(void);
extern int sys_consoleclear(void);
extern int sys_systicks(void);
extern int sys_procticks(void);
extern int sys_procs(void);
extern int sys_clone(void);
extern int sys_join(void);
extern int sys_detach(void);
extern int sys_ktotalbytes(void);
extern int sys_kfreebytes(void);
extern int sys_kusedbytes(void);

static int (*syscalls[])(void) =
{
    [SYS_fork]          sys_fork,
    [SYS_exit]          sys_exit,
    [SYS_wait]          sys_wait,
    [SYS_pipe]          sys_pipe,
    [SYS_read]          sys_read,
    [SYS_kill]          sys_kill,
    [SYS_exec]          sys_exec,
    [SYS_fstat]         sys_fstat,
    [SYS_chdir]         sys_chdir,
    [SYS_dup]           sys_dup,
    [SYS_getpid]        sys_getpid,
    [SYS_sbrk]          sys_sbrk,
    [SYS_sleep]         sys_sleep,
    [SYS_uptime]        sys_uptime,
    [SYS_open]          sys_open,
    [SYS_write]         sys_write,
    [SYS_mknod]         sys_mknod,
    [SYS_unlink]        sys_unlink,
    [SYS_link]          sys_link,
    [SYS_mkdir]         sys_mkdir,
    [SYS_close]         sys_close,
    [SYS_consolecolor]  sys_consolecolor,
    [SYS_consoleputsxy] sys_consoleputsxy,
    [SYS_consolebox]    sys_consolebox,
    [SYS_consolewindow] sys_consolewindow,
    [SYS_consoleclear]  sys_consoleclear,
    [SYS_systicks]      sys_systicks,
    [SYS_procticks]     sys_procticks,
    [SYS_procs]         sys_procs,
    [SYS_clone]         sys_clone,
    [SYS_join]          sys_join,
    [SYS_detach]        sys_detach,
    [SYS_ktotalbytes]   sys_ktotalbytes,
    [SYS_kfreebytes]    sys_kfreebytes,
    [SYS_kusedbytes]    sys_kusedbytes,
};

void syscall(void)
{
    struct proc *curproc = myproc();

    int num = curproc->tf->eax;

    if (num > 0x00 && num < NELEM(syscalls) && syscalls[num])
    {
        curproc->tf->eax = syscalls[num]();
    }
    else
    {
        cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name, num);
        curproc->tf->eax = -0x01;
    }
}
