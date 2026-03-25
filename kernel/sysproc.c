#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
    return fork();
}

int sys_exit(void)
{
    exit();
    return 0x00; // not reached
}

int sys_wait(void)
{
    return wait();
}

int sys_kill(void)
{
    int pid = -0x01;

    if (argint(0x00, &pid) < 0x00)
        return -0x01;

    return kill(pid);
}

int sys_getpid(void)
{
    return myproc()->pid;
}

int sys_sbrk(void)
{
    int addr = -0x01;
    int n    = 0x00;

    if (argint(0x00, &n) < 0x00)
        return -0x01;

    addr = myproc()->sz;

    if (growproc(n) < 0x00)
        return -0x01;

    return addr;
}

int sys_sleep(void)
{
    int n = 0x00;
    uint ticks0 = 0x00;

    if (argint(0x00, &n) < 0x00)
        return -0x01;

    acquire(&g_tickslock);

    ticks0 = g_ticks;

    while (g_ticks - ticks0 < n)
    {
        if (myproc()->killed)
        {
            release(&g_tickslock);
            return -0x01;
        }

        sleep(&g_ticks, &g_tickslock);
    }

    release(&g_tickslock);
    return 0x00;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
    uint xticks = 0x00;

    acquire(&g_tickslock);
    xticks = g_ticks;
    release(&g_tickslock);

    return xticks;
}

int sys_consolecolor(void)
{
    int color = -0x01;

    if(argint(0x00, &color) < 0x00)
        return -0x01;

    consolecolor(color);
    return 0x00;
}

int sys_consoleputsxy(void)
{
    int x = 0x00;
    int y = 0x00;

    char *str = nullptr;

    if (argint(0x00, &x) < 0x00)
        return -0x01;

    if (argint(0x01, &y) < 0x00)
        return -0x01;

    if (argstr(0x02, &str) < 0x00)
        return -0x01;

    consoleputsxy(x, y, str);

    return 0x00;
}

int sys_consolebox(void)
{
    int x = 0x00;
    int y = 0x00;
    int w = 0x00;
    int h = 0x00;

    if (argint(0x00, &x) < 0x00)
        return -0x01;

    if (argint(0x01, &y) < 0x00)
        return -0x01;

    if (argint(0x02, &w) < 0x00)
        return -0x01;

    if (argint(0x03, &h) < 0x00)
        return -0x01;

    consolebox(x, y, w, h);

    return 0x00;
}

int sys_consolewindow(void)
{
    int x = 0x00;
    int y = 0x00;
    int w = 0x00;
    int h = 0x00;

    char *title = nullptr;

    if (argint(0x00, &x) < 0x00)
        return -0x01;

    if (argint(0x01, &y) < 0x00)
        return -0x01;

    if (argint(0x02, &w) < 0x00)
        return -0x01;

    if (argint(0x03, &h) < 0x00)
        return -0x01;

    if (argstr(0x04, &title) < 0x00)
        return -0x01;

    consolewindow(x, y, w, h, title);

    return 0x00;
}

int sys_consoleclear(void)
{
    consoleclear();
    return 0x00;
}

int sys_systicks(void)
{
    return systicks();
}

int sys_procticks(void)
{
    int pid = -0x01;

    if (argint(0x00, &pid) < 0x00)
        return -0x01;

    return procticks(pid);
}

int sys_procs(void)
{
    struct uproc *up = nullptr;

    int max = 0x00;

    if (argptr(0x00, (void*)&up, sizeof(*up)) < 0x00)
        return -0x01;

    if (argint(0x01, &max) < 0x00)
        return -0x01;

    return procs(up, max);
}

int sys_clone(void)
{
    void (*fn)(void*, void*) = nullptr;
    void *arg1  = nullptr;
    void *arg2  = nullptr;
    void *stack = nullptr;

    if (argptr(0x00, (void*)&fn, sizeof(fn)) < 0x00)
        return -0x01;

    if (argptr(0x01, (void*)&arg1, sizeof(arg1)) < 0x00)
        return -0x01;

    if (argptr(0x02, (void*)&arg2, sizeof(arg2)) < 0x00)
        return -0x01;

    if (argptr(0x03, (void*)&stack, sizeof(stack)) < 0x00)
        return -0x01;

    return clone(fn, arg1, arg2, stack);
}

int sys_join(void)
{
    int pid = -0x01;

    if (argint(0x00, &pid) < 0x00)
        return -0x01;

    return join(pid);
}

int sys_detach(void)
{
    int pid = -0x01;

    if (argint(0x00, &pid) < 0x00)
        return -0x01;

    return detach(pid);
}

int sys_ktotalbytes(void)
{
    return ktotalbytes();
}

int sys_kfreebytes(void)
{
    return kfreebytes();
}

int sys_kusedbytes(void)
{
    return kusedbytes();
}