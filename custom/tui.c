#include "types.h"
#include "stat.h"
#include "user.h"

#include "vga_color.h"

void tui_demo()
{
    clear_window();

    consolecolor(VGA_CYAN);
    draw_box(0x01, 0x01, 78, 23);

    consolecolor(VGA_YELLOW);
    draw_string(0x02, 0x01, "TUI PROFILER (xv6)");

    consolecolor(VGA_GREEN);
    draw_window(0x02, 0x03, 35, 0x0A, "SYSTEM");

    consolecolor(VGA_WHITE);
    draw_string(0x04, 0x05, "CPU : [OK]");
    draw_string(0x04, 0x06, "MEM : [OK]");
    draw_string(0x04, 0x07, "DISK: [OK]");

    consolecolor(VGA_RED);
    draw_window(0x2A, 0x03, 35, 0x0A, "NETWORK");
    
    consolecolor(VGA_WHITE);
    draw_string(0x2C, 0x05, "STATUS : [-]");
    draw_string(0x2C, 0x06, "LATENCY: [-ms]");

    consolecolor(VGA_BLUE);
    draw_window(0x02, 0x0E, 75, 0x09, "LOG");

    consolecolor(VGA_WHITE);
    draw_string(0x04, 0x10, "[INFO] system started");
    draw_string(0x04, 0x11, "[WARN] network unstable");
    draw_string(0x04, 0x12, "[ERROR] connection lost");

    consolecolor(VGA_LIGHT_GREY);
}

int main(int argc, char *argv[])
{
    tui_demo();

    exit();
}