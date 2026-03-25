#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#include "vga_color.h"

#define BUF_SIZE 4096

static char s_buffer[BUF_SIZE] = { 0x00 };
static char s_bytes = 0x00;

enum VI_MODE
{
    NORMAL = 0x00,
    INSERT = 0x01,
};

int open_file(const char *filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd < 0x00)
        return -0x01;

    s_bytes = read(fd, s_buffer, BUF_SIZE - 0x01);

    if (s_bytes < 0x00)
        s_bytes = 0x00;

    close(fd);
    return s_bytes;
}

void save_file(const char *filename)
{
    int fd = open(filename, O_CREATE | O_WRONLY);

    if (fd < 0x00)
    {
        printf(0x02, "[LVL: ERROR][PROC: VI][FUNC: %s][LINE: %d] save failed\n", __func__, __LINE__);
        return;
    }

    write(fd, s_buffer, s_bytes);
    close(fd);

    printf(0x01, "\n[SAVED]\n");
}

void display_file()
{
    for (int i = 0x00; i < s_bytes; i++)
    {
        printf(0x01, "%c", s_buffer[i]);
    }
}

int main(int argc, char *argv[])
{
    enum VI_MODE mode    = NORMAL;

    const char *filename = nullptr;
    int bytes = 0x00;

    if (argc <= 0x01)
    {
        consolecolor(VGA_BROWN);
        printf(0x01, "usage: vi <filename>\n");
        consolecolor(VGA_LIGHT_GREY);
        exit();
    }

    filename = argv[0x01];
    bytes    = open_file(filename);

    consoleclear();
    printf(0x01, "#\n");
    printf(0x01, "##################################################\n");
    printf(0x01, "            vi(xv6) - visual interface            \n");
    printf(0x01, "##################################################\n");
    printf(0x01, "version      : 1.0\n");
    printf(0x01, "date         : 2026-03-25\n");
    printf(0x01, "--------------------------------------------------\n");
    printf(0x01, "* file name  : %s\n", filename);
    printf(0x01, "* total bytes: %d\n", BUF_SIZE);
    printf(0x01, "* read bytes : %d\n", bytes);
    printf(0x01, "* mode       : %s\n", mode == NORMAL ? "NORMAL" : "INSERT");
    printf(0x01, "--------------------------------------------------\n");

    if (bytes > 0x00)
        display_file();

    while (true)
    {
        char c = 0x00;

        if (read(0x00, &c, 0x01) <= 0x00)
            continue;

        if (mode == NORMAL)
        {
            if (c == 'i')
            {
                mode = INSERT;
                printf(0x01, "\n[INSERT]\n");
            }
            else if (c == 'q')
            {
                printf(0x01, "\n[EXIT]\n");
                break;
            }
            else if (c == 'w')
            {
                save_file(filename);
            }
        }
        else if (mode == INSERT)
        {
            if (c == 0x1B) // ESC
            {
                mode = NORMAL;
                printf(0x01, "\n[NORMAL]\n");
                continue;
            }
            else if (c == 0x08 || c == 0x7F)
            {
                if (s_bytes > 0x00)
                {
                    s_bytes--;
                    printf(0x01, "\b \b");
                }

                continue;
            }
            else if (c == '\r' || c == '\n')
            {
                static bool once = false;

                if (once == false && s_bytes <= 0x00)
                {
                    once = true;
                    continue;
                }

                c = '\n';
            }

            if (s_bytes < BUF_SIZE - 0x01)
                s_buffer[s_bytes++] = c;
        }
    }

    exit();
}