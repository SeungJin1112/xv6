#include "types.h"
#include "stat.h"
#include "user.h"

#include "vga_color.h"

static void putc(int fd, char c)
{
    write(fd, &c, 0x01);
}

static void printint(int fd, int xx, int base, int sgn)
{
    static char digits[] = "0123456789ABCDEF";

    char buffer[0x10] = { 0x00 };
    int  idx = 0x00;

    bool neg = false;
    uint x   = xx;

    if (sgn && xx < 0x00)
    {
        neg = true;
        x   = -xx;
    }

    idx = 0x00;

    do
    {
        buffer[idx++] = digits[x % base];
    } while ((x /= base) != 0x00);

    if (neg == true)
        buffer[idx++] = '-';

    while (--idx >= 0x00)
    {
        putc(fd, buffer[idx]);
    }
}

void printf(int fd, const char *fmt, ...)
{
    char *s   = nullptr;

    int c     = 0x00;
    int i     = 0x00;
    int state = 0x00;

    uint *ap  = (uint *)(void *)&fmt + 0x01;

    for (i = 0x00; fmt[i]; i++)
    {
        c = fmt[i] & 0xff;

        if (state == 0x00)
        {
            if (c == '%')
                state = '%';
            else
                putc(fd, c);
        }
        else if (state == '%')
        {
            //--------------------------------------------------
            if (c == 'd')
            {
                printint(fd, *ap, 0x0A, 0x01);
                ap++;
            }
            //--------------------------------------------------
            else if (c == 'x' || c == 'p')
            {
                printint(fd, *ap, 0x10, 0x00);
                ap++;
            }
            //--------------------------------------------------
            else if (c == 's')
            {
                s = (char *)*ap;
                ap++;

                if (s == nullptr)
                    s = "(null)";

                while (*s != nullptr)
                {
                    putc(fd, *s);
                    s++;
                }
            }
            //--------------------------------------------------
            else if (c == 'R') consolecolor(VGA_RED);
            else if (c == 'G') consolecolor(VGA_GREEN);
            else if (c == 'B') consolecolor(VGA_BLUE);
            else if (c == 'Y') consolecolor(VGA_YELLOW);
            else if (c == 'W') consolecolor(VGA_WHITE);
            else if (c == 'C') consolecolor(VGA_CYAN);
            else if (c == 'M') consolecolor(VGA_MAGENTA);
            else if (c == '-') consolecolor(VGA_LIGHT_GREY);
            //--------------------------------------------------
            else if (c == 'c')
            {
                putc(fd, *ap);
                ap++;
            }
            //--------------------------------------------------
            else if (c == '%')
            {
                putc(fd, c);
            }
            //--------------------------------------------------
            else
            {
                putc(fd, '%');
                putc(fd, c);
            }
            //--------------------------------------------------
            state = 0x00;
        }
    }
}