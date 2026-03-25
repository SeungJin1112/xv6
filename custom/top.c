#include "types.h"
#include "stat.h"
#include "user.h"

#include "vga_color.h"

static struct uproc s_plist[64] = { 0x00 };
static int s_pcount = 0x00;

void top_init()
{
    consoleclear();
    consolecolor(VGA_LIGHT_GREY);
    consolebox(0x01, 0x01, 78, 23);
    consoleputsxy(0x02, 0x01, "");
    consolecolor(VGA_LIGHT_GREY);
}

void top_header()
{
    consolecolor(VGA_WHITE);
    consolebox(0x02, 0x02, 75, 0x03);
    consolecolor(VGA_CYAN);
    consoleputsxy(0x04, 0x03, "top(xv6) - system monitor");
    consolecolor(VGA_LIGHT_GREY);
    consoleputsxy(0x25, 0x03, "version: 1.0");
    consoleputsxy(0x38, 0x03, "date: 2026-03-24");
    consolecolor(VGA_LIGHT_GREY);
}

int get_cpu_usage()
{
    static int prev = 0x00;
    static int avg  = 0x00;

    int now   = systicks();
    int delta = now - prev;
    int percent = delta / 0x0A;

    prev = now;

    if (percent > 100)  percent = 100;
    if (percent < 0x00) percent = 0x00;

    avg = (avg * 0x07 + percent * 0x03) / 0x0A;

    return avg;
}

int get_mem_usage()
{
    int mem_total = ktotalbytes();
    // int mem_free  = kfreebytes();
    int mem_used  = kusedbytes();

    return (mem_used * 100) / mem_total;
}

void top_body_1()
{
    char cpu_usage[0x0A] = { 0x00 };
    char mem_usage[0x0A] = { 0x00 };
    char tmp[0x05]       = { 0x00 };
    int  percent         = 0x00;
    //--------------------------------------------------
    percent = get_cpu_usage();

    itoa(percent, tmp);
    strcat(cpu_usage, "[");
    strcat(cpu_usage, tmp);
    strcat(cpu_usage, "%]");
    memset(tmp, 0x00, sizeof(tmp));
    //--------------------------------------------------
    percent = get_mem_usage();

    itoa(percent, tmp);
    strcat(mem_usage, "[");
    strcat(mem_usage, tmp);
    strcat(mem_usage, "%]");
    memset(tmp, 0x00, sizeof(tmp));
    //--------------------------------------------------
    consolecolor(VGA_WHITE);
    consolebox(0x02, 0x05, (75 / 0x02), 0x06);
    consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04, 0x06, "cpu usage:");    /*value*/ consolecolor(VGA_LIGHT_GREY); consoleputsxy(0x20, 0x06, cpu_usage); consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04, 0x07, "memory usage:"); /*value*/ consolecolor(VGA_LIGHT_GREY); consoleputsxy(0x20, 0x07, mem_usage); consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04, 0x08, "disk usage:");   /*value*/ consolecolor(VGA_RED);        consoleputsxy(0x20, 0x08, "[x]");     consolecolor(VGA_CYAN);
    consolecolor(VGA_LIGHT_GREY);
}

void top_body_2()
{
    //--------------------------------------------------
    int states[0x07] = { 0x00 };

    for (int i = 0x00; i < s_pcount; i++)
    {
        if (s_plist[i].state == 0x00) states[0x00]++; // unused
        if (s_plist[i].state == 0x01) states[0x01]++; // embryo
        if (s_plist[i].state == 0x02) states[0x02]++; // sleeping
        if (s_plist[i].state == 0x03) states[0x03]++; // runnable
        if (s_plist[i].state == 0x04) states[0x04]++; // running
        if (s_plist[i].state == 0x05) states[0x05]++; // zombie
    }
    //--------------------------------------------------
    char running[0x0A]  = { 0x00 };
    char sleeping[0x0A] = { 0x00 };
    char zombie[0x0A]   = { 0x00 };
    char tmp[0x05]      = { 0x00 };

    itoa(states[0x04], tmp);
    strcat(running, "[");
    strcat(running, tmp);
    strcat(running, "]");
    memset(tmp, 0x00, sizeof(tmp));

    itoa(states[0x02], tmp);
    strcat(sleeping, "[");
    strcat(sleeping, tmp);
    strcat(sleeping, "]");
    memset(tmp, 0x00, sizeof(tmp));

    itoa(states[0x05], tmp);
    strcat(zombie, "[");
    strcat(zombie, tmp);
    strcat(zombie, "]");
    memset(tmp, 0x00, sizeof(tmp));
    //--------------------------------------------------
    consolecolor(VGA_WHITE);
    consolebox(0x02 + (75 / 0x02) + 0x01, 0x05, (75 / 0x02), 0x06);
    consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04 + (75 / 0x02) + 0x01, 0x06, "running:");  /*value*/ consolecolor(VGA_LIGHT_GREY); consoleputsxy(0x20 + (75 / 0x02) + 0x01, 0x06, running);  consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04 + (75 / 0x02) + 0x01, 0x07, "sleeping:"); /*value*/ consolecolor(VGA_LIGHT_GREY); consoleputsxy(0x20 + (75 / 0x02) + 0x01, 0x07, sleeping); consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04 + (75 / 0x02) + 0x01, 0x08, "zombie:");   /*value*/ consolecolor(VGA_LIGHT_GREY); consoleputsxy(0x20 + (75 / 0x02) + 0x01, 0x08, zombie);   consolecolor(VGA_CYAN);
    /*key*/ consoleputsxy(0x04 + (75 / 0x02) + 0x01, 0x09, "uptime:");   /*value*/ consolecolor(VGA_BROWN);      consoleputsxy(0x20 + (75 / 0x02) + 0x01, 0x09, "[-]");    consolecolor(VGA_CYAN);
    consolecolor(VGA_LIGHT_GREY);
}

int find_depth(struct uproc *list, int n, int ppid)
{
    int depth = 0x00;

    while (ppid != 0x00)
    {
        bool found = false;

        for (int i = 0x00; i < n; i++)
        {
            if (list[i].pid == ppid)
            {
                found = true;
                ppid  = list[i].ppid;
                depth++;
                break;
            }
        }

        if (found == false)
            break;
    }

    return depth;
}

void top_body_3()
{
    int y = 0x0C;

    consolecolor(VGA_WHITE);
    consolewindow(0x02, 0x0B, 75, 0x0C, "processes [name | pid | ppid | cpu | mem | time]");
    consolecolor(VGA_LIGHT_GREY);

    for (int i = 0x00; i < s_pcount; i++)
    {
        char line[64]  = { 0x00 };
        char tmp[0x08] = { 0x00 };

        int idx       = 0x00;
        int depth     = find_depth(s_plist, s_pcount, s_plist[i].ppid);

        int pid       = s_plist[i].pid;
        int ppid      = s_plist[i].ppid;
        int cpu_usage = s_plist[i].cpu_usage;
        int mem_usage = s_plist[i].mem_usage / 1024;
        int cpu_time  = s_plist[i].cpu_time;

        for (int d = 0x00; d < depth; d++)
        {
            line[idx++] = ' ';
            line[idx++] = ' ';
        }

        if (depth > 0x00)
        {
            line[idx++] = '|';
            line[idx++] = '-';
            line[idx++] = ' ';
        }

        for (int j = 0x00; s_plist[i].name[j]; j++)
        {
            line[idx++] = s_plist[i].name[j];
        }

        while (idx < 50)
        {
            line[idx++] = ' ';
        }

        itoa(pid, tmp);
        for (int j = 0x00; tmp[j]; j++) { line[idx++] = tmp[j]; }
        line[idx++] = ' ';
        memset(tmp, 0x00, sizeof(tmp));

        itoa(ppid, tmp);
        for (int j = 0x00; tmp[j]; j++) { line[idx++] = tmp[j]; }
        line[idx++] = ' ';
        memset(tmp, 0x00, sizeof(tmp));

        itoa(cpu_usage, tmp);
        for (int j = 0x00; tmp[j]; j++) { line[idx++] = tmp[j]; }
        line[idx++] = '%';
        line[idx++] = ' ';
        memset(tmp, 0x00, sizeof(tmp));

        itoa(mem_usage, tmp);
        for (int j = 0x00; tmp[j]; j++) { line[idx++] = tmp[j]; }
        line[idx++] = 'K';
        line[idx++] = ' ';
        memset(tmp, 0x00, sizeof(tmp));

        itoa(cpu_time, tmp);
        for (int j = 0x00; tmp[j]; j++) { line[idx++] = tmp[j]; }
        line[idx] = 0x00;

        if      (s_plist[i].state == 0x04) consolecolor(VGA_LIGHT_GREEN); // running
        else if (s_plist[i].state == 0x02) consolecolor(VGA_LIGHT_GREY);  // sleeping
        else if (s_plist[i].state == 0x05) consolecolor(VGA_RED);         // zombie
        else                               consolecolor(VGA_LIGHT_GREY);  // other

        consoleputsxy(0x04, y++, line);

        if (y > 0x16)
            break;
    }

    consolecolor(VGA_LIGHT_GREY);
}

void top_footer()
{
}

void top(bool force)
{
    static bool once = false;

    if (once == false || force)
    {
        top_init();
        once = true;
    }
    //--------------------------------------------------
    top_header();
    //--------------------------------------------------
    top_body_1(); /* | */ top_body_2();
    top_body_3();
    //--------------------------------------------------
    top_footer();
    //--------------------------------------------------
}

static bool s_running = false;

void worker(void *arg1, void *arg2)
{
    bool force = false;
    int pcount = 0x00;

    while (s_running)
    {
        force  = false;
        pcount = procs(s_plist, sizeof(s_plist) / sizeof(s_plist[0x00]));

        if (s_pcount != pcount)
        {
            force    = true;
            s_pcount = pcount;
        }

        top(force);
        sleep(100);
    }

    consoleclear();
    exit();
}

int main(int argc, char *argv[])
{
    char buffer[0x01] = { 0x00 };

    void *stack = malloc(4096);
    int pid     = -0x01;
    s_running   = true;

    pid = clone(worker, (void*)0x00, (void*)0x00, stack);

    while (true)
    {
        read(0x00, buffer, 0x01);

        if (buffer[0x00] == 'q')
        {
            s_running = false;
            join(pid);
            break;
        }
    }

    if (stack != nullptr)
        free(stack);

    exit();
}