#include "types.h"
#include "stat.h"
#include "user.h"

#include "vga_color.h"

ushort parse_color(const char *arg)
{
    ushort color = VGA_LIGHT_GREY;

    const char *value = arg;

    if      (value == nullptr)                      color = VGA_LIGHT_GREY;
    else if (strcmp(value, "black")        == 0x00) color = VGA_BLACK;
    else if (strcmp(value, "blue")         == 0x00) color = VGA_BLUE;
    else if (strcmp(value, "darkblue")     == 0x00) color = VGA_DARK_BLUE;
    else if (strcmp(value, "green")        == 0x00) color = VGA_GREEN;
    else if (strcmp(value, "cyan")         == 0x00) color = VGA_CYAN;
    else if (strcmp(value, "red")          == 0x00) color = VGA_RED;
    else if (strcmp(value, "magenta")      == 0x00) color = VGA_MAGENTA;
    else if (strcmp(value, "brown")        == 0x00) color = VGA_BROWN;
    else if (strcmp(value, "orange")       == 0x00) color = VGA_ORANGE;
    else if (strcmp(value, "lightgrey")    == 0x00) color = VGA_LIGHT_GREY;
    else if (strcmp(value, "darkgrey")     == 0x00) color = VGA_DARK_GREY;
    else if (strcmp(value, "lightblue")    == 0x00) color = VGA_LIGHT_BLUE;
    else if (strcmp(value, "lightgreen")   == 0x00) color = VGA_LIGHT_GREEN;
    else if (strcmp(value, "lightcyan")    == 0x00) color = VGA_LIGHT_CYAN;
    else if (strcmp(value, "lightred")     == 0x00) color = VGA_LIGHT_RED;
    else if (strcmp(value, "lightmagenta") == 0x00) color = VGA_LIGHT_MAGENTA;
    else if (strcmp(value, "white")        == 0x00) color = VGA_WHITE;
    else if (strcmp(value, "yellow")       == 0x00) color = VGA_YELLOW;
    else                                            color = VGA_LIGHT_GREY;

    return color;
}

int main(int argc, char **argv)
{
    ushort color = VGA_LIGHT_GREY;

    for (int i = 0x01; i < argc; i++)
    {
        if (strncmp(argv[i], "--color=", 0x08) == 0x00)
        {
            const char *value = argv[i] + 0x08;
            color = parse_color(value);
            break;
        }
    }

    consolecolor(color);

    exit();
}