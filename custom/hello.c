#include "types.h"
#include "stat.h"
#include "user.h"

#include "vga_color.h"

int main(int argc, char **argv)
{
    consolecolor(VGA_ORANGE);
    printf(0x01, "Hello, %R world %G !\n");
    consolecolor(VGA_WHITE);

    exit();
}