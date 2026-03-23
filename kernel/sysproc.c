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

    acquire(&tickslock);

    ticks0 = ticks;

    while (ticks - ticks0 < n)
    {
        if (myproc()->killed)
        {
            release(&tickslock);
            return -0x01;
        }

        sleep(&ticks, &tickslock);
    }

    release(&tickslock);
    return 0x00;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
    uint xticks = 0x00;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);

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

int sys_draw_string(void)
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

    draw_string(x, y, str);

    return 0x00;
}

int sys_draw_box(void)
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

    draw_box(x, y, w, h);

    return 0x00;
}

int sys_draw_window(void)
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

    draw_window(x, y, w, h, title);

    return 0x00;
}

int sys_clear_window(void)
{
    clear_window();
    return 0x00;
}