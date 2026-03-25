// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#include "vga_color.h"

static void consputc(int);

static int s_panicked = 0x00;

static struct
{
    struct spinlock lock;
    int locking;
} s_cons;

static void printint(int xx, int base, int sign)
{
    static char digits[] = "0123456789abcdef";

    char buffer[0x10] = { 0x00 };
    int  i = 0x00;
    uint x = xx;

    if (sign && (sign = xx < 0x00))
        x = -xx;

    do
    {
        buffer[i++] = digits[x % base];
    } while ((x /= base) != 0x00);

    if (sign)
        buffer[i++] = '-';

    while (--i >= 0x00)
    {
        consputc(buffer[i]);
    }
}
// PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...)
{
    uint *argp  = nullptr;
    char *s     = nullptr;

    int c       = 0x00;
    int locking = s_cons.locking;

    if (locking)
        acquire(&s_cons.lock);

    if (fmt == 0x00)
        panic("null fmt");

    argp = (uint *)(void *)(&fmt + 0x01);

    for (int i = 0x00; (c = fmt[i] & 0xff) != 0x00; i++)
    {
        if (c != '%')
        {
            consputc(c);
            continue;
        }

        c = fmt[++i] & 0xff;

        if (c == 0x00)
            break;

        switch (c)
        {
        //--------------------------------------------------
        case 'd':
            printint(*argp++, 0x0A, 0x01);
            break;
        //--------------------------------------------------
        case 'x':
        case 'p':
            printint(*argp++, 0x10, 0x00);
            break;
        //--------------------------------------------------
        case 's':
            if ((s = (char *)*argp++) == 0x00)
                s = "(null)";

            for (; *s; s++)
                consputc(*s);
            break;
        //--------------------------------------------------
        case '%':
            consputc('%');
            break;
        //--------------------------------------------------
        case 'R': consolecolor(VGA_RED);        break;
        case 'G': consolecolor(VGA_GREEN);      break;
        case 'B': consolecolor(VGA_BLUE);       break;
        case 'Y': consolecolor(VGA_YELLOW);     break;
        case 'W': consolecolor(VGA_WHITE);      break;
        case 'C': consolecolor(VGA_CYAN);       break;
        case 'M': consolecolor(VGA_MAGENTA);    break;
        case '-': consolecolor(VGA_LIGHT_GREY); break;
        //--------------------------------------------------
        default:
            // Print unknown % sequence to draw attention.
            consputc('%');
            consputc(c);
            break;
        //--------------------------------------------------
        }
    }

    if (locking)
        release(&s_cons.lock);
}

void panic(char *s)
{
    int i = 0x00;
    uint pcs[0x0A] = { 0x00 };

    cli();
    s_cons.locking = 0x00;
    // use lapiccpunum so that we can call panic from mycpu()
    cprintf("lapicid %d: panic: ", lapicid());
    cprintf(s);
    cprintf("\n");
    getcallerpcs(&s, pcs);

    for (i = 0; i < 0x0A; i++)
        cprintf(" %p", pcs[i]);

    s_panicked = 0x01; // freeze other CPU

    for (;;)
        ;
}

// PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT   0x3d4

static ushort *s_crt      = (ushort *)P2V(0xb8000); // CGA memory

static ushort s_vga_color = (ushort)VGA_LIGHT_GREY;

static void cgaputc(int c)
{
    int pos = 0x00;

    // Cursor position: col + 80*row.
    outb(CRTPORT, 0x0E);
    pos  = inb(CRTPORT + 0x01) << 0x08;
    outb(CRTPORT, 0x0F);
    pos |= inb(CRTPORT + 0x01);

    if (c == '\n')
    {
        pos += 80 - pos % 80;
    }
    else if (c == BACKSPACE)
    {
        if (pos > 0x00)
            --pos;
    }
    else
    {
        s_crt[pos++] = (c & 0xff) | s_vga_color;
    }

    if (pos < 0x00 || pos > 25 * 80)
        panic("pos under/overflow");

    if ((pos / 80) >= 24)
    { // Scroll up.
        memmove(s_crt, s_crt + 80, sizeof(s_crt[0x00]) * 23 * 80);
        pos -= 80;
        memset(s_crt + pos, 0x00, sizeof(s_crt[0x00]) * (24 * 80 - pos));
    }

    outb(CRTPORT,  0x0E);
    outb(CRTPORT + 0x01, pos >> 0x08);
    outb(CRTPORT,  0x0F);
    outb(CRTPORT + 0x01, pos);

    s_crt[pos] = ' ' | s_vga_color;
}

void consputc(int c)
{
    if (s_panicked)
    {
        cli();
        for (;;)
            ;
    }

    if (c == BACKSPACE)
    {
        uartputc('\b');
        uartputc(' ');
        uartputc('\b');
    }
    else
    {
        uartputc(c);
    }

    cgaputc(c);
}

#define INPUT_BUF 128

struct
{
    char buf[INPUT_BUF];
    uint r; // Read index
    uint w; // Write index
    uint e; // Edit index
} g_input;

#define C(x) ((x) - '@') // Control-x

void consoleintr(int (*getc)(void))
{
    int c = 0x00;
    int doprocdump = 0x00;

    acquire(&s_cons.lock);

    while ((c = getc()) >= 0x00)
    {
        switch (c)
        {
        case C('P'): // Process listing.
            // procdump() locks cons.lock indirectly; invoke later
            doprocdump = 0x01;
            break;

        case C('U'): // Kill line.
            while (g_input.e != g_input.w &&
                   g_input.buf[(g_input.e - 1) % INPUT_BUF] != '\n')
            {
                g_input.e--;
                consputc(BACKSPACE);
            }
            break;

        case C('H'):
        case '\x7f': // Backspace
            if (g_input.e != g_input.w)
            {
                g_input.e--;
                consputc(BACKSPACE);
            }
            break;

        default:
            if (c != 0x00 && g_input.e - g_input.r < INPUT_BUF)
            {
                c = (c == '\r') ? '\n' : c;
                g_input.buf[g_input.e++ % INPUT_BUF] = c;

                consputc(c);

                if (c == '\n' || c == C('D') || g_input.e == g_input.r + INPUT_BUF)
                {
                    g_input.w = g_input.e;
                    wakeup(&g_input.r);
                }
            }
            break;
        }
    }

    release(&s_cons.lock);

    if (doprocdump)
        procdump(); // now call procdump() wo. cons.lock held
}

int consoleread(struct inode *ip, char *dst, int n)
{
    uint original_bytes = 0x00;

    int c = 0x00;

    iunlock(ip);
    original_bytes = n;
    acquire(&s_cons.lock);

    while (n > 0x00)
    {
        while (g_input.r == g_input.w)
        {
            if (myproc()->killed)
            {
                release(&s_cons.lock);
                ilock(ip);
                return -0x01;
            }

            sleep(&g_input.r, &s_cons.lock);
        }

        c = g_input.buf[g_input.r++ % INPUT_BUF];

        if (c == C('D'))
        { // EOF
            if (n < original_bytes)
            {
                // Save ^D for next time, to make sure
                // caller gets a 0-byte result.
                g_input.r--;
            }

            break;
        }

        *dst++ = c;
        --n;

        if (c == '\n')
            break;
    }

    release(&s_cons.lock);
    ilock(ip);

    return original_bytes - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{
    iunlock(ip);
    acquire(&s_cons.lock);

    for (int i = 0x00; i < n; i++)
    {
        consputc(buf[i] & 0xff);
    }

    release(&s_cons.lock);
    ilock(ip);
    return n;
}

void consoleinit(void)
{
    initlock(&s_cons.lock, "console");

    devsw[CONSOLE].write = consolewrite;
    devsw[CONSOLE].read  = consoleread;
    s_cons.locking = 0x01;

    ioapicenable(IRQ_KBD, 0x00);
}

void consolecolor(ushort color)
{
    s_vga_color = color;
}

#define WIDTH  80
#define HEIGHT 25

void consolegotoxy(int x, int y)
{
    int pos = y * WIDTH + x;

    outb(CRTPORT,  0x0E);
    outb(CRTPORT + 0x01, pos >> 0x08);
    outb(CRTPORT,  0x0F);
    outb(CRTPORT + 0x01, pos);
}

void consoleputcxy(int x, int y, char c)
{
    consolegotoxy(x, y);
    consputc(c);
}

void consoleputsxy(int x, int y, const char* s)
{
    consolegotoxy(x, y);

    while (*s)
    {
        consputc(*s++);
    }
}

void consolebox(int x, int y, int w, int h)
{
    if (w > WIDTH)  w = WIDTH;
    if (h > HEIGHT) h = HEIGHT;

    // top / bottom
    for (int i = 0x00; i < w; i++)
    {
        consoleputcxy(x + i, y, '-');
        consoleputcxy(x + i, y + h - 0x01, '-');
    }

    // left / right
    for (int i = 0x00; i < h; i++)
    {
        consoleputcxy(x, y + i, '|');
        consoleputcxy(x + w - 0x01, y + i, '|');
    }

    // corners
    consoleputcxy(x, y, '+');
    consoleputcxy(x + w - 0x01, y, '+');
    consoleputcxy(x, y + h - 0x01, '+');
    consoleputcxy(x + w - 0x01, y + h - 0x01, '+');
}

void consolewindow(int x, int y, int w, int h, const char* title)
{
    consolebox(x, y, w, h);

    if (title)
        consoleputsxy(x + 0x02, y, title);
}

void consoleclear()
{
    for (int y = 0x00; y < (HEIGHT - 0x01); y++)
    {
        for (int x = 0x00; x < WIDTH; x++)
        {
            consoleputcxy(x, y, ' ');
        }
    }
}