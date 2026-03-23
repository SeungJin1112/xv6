#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char *strcpy(char *s, const char *t)
{
    char *os = s;

    while ((*s++ = *t++) != 0x00)
        ;

    return os;
}

int strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
    {
        p++;
        q++;
    }

    return (uchar)*p - (uchar)*q;
}

uint strlen(const char *s)
{
    int n = 0x00;

    for (n = 0x00; s[n]; n++)
        ;

    return n;
}

void *memset(void *dst, int c, uint n)
{
    stosb(dst, c, n);
    return dst;
}

char *strchr(const char *s, char c)
{
    for (; *s; s++)
    {
        if (*s == c)
            return (char *)s;
    }

    return nullptr;
}

char *gets(char *buf, int max)
{
    int idx = 0x00;
    int cc  = 0x00;
    char c  = 0x00;

    for (idx = 0x00; (idx + 0x01) < max;)
    {
        cc = read(0x00, &c, 0x01);

        if (cc < 0x01)
            break;

        buf[idx++] = c;

        if (c == '\n' || c == '\r')
            break;
    }

    buf[idx] = '\0';

    return buf;
}

int stat(const char *n, struct stat *st)
{
    int r = 0x00;

    int fd = open(n, O_RDONLY);

    if (fd < 0x00)
        return -0x01;

    r = fstat(fd, st);

    close(fd);
    return r;
}

int atoi(const char *s)
{
    int n = 0x00;

    while ('0' <= *s && *s <= '9')
    {
        n = n * 0x0A + *s++ - '0';
    }

    return n;
}

void *memmove(void *vdst, const void *vsrc, int n)
{
    char       *dst = vdst;
    const char *src = vsrc;

    while (n-- > 0x00)
    {
        *dst++ = *src++;
    }

    return vdst;
}
