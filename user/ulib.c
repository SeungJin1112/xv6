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

int strncmp(const char *p, const char *q, int n)
{
    while (n-- > 0x00 && *p && *q)
    {
        if (*p != *q)
            return *p - *q;

        p++;
        q++;
    }

    return 0x00;
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

void itoa(int src, char *dst)
{
    int idx = 0x00;
    int neg = 0x00;

    if (src == 0x00)
    {
        dst[idx++] = '0';
        dst[idx]   = 0x00;
        return;
    }

    if (src < 0x00)
    {
        neg = 0x01;
        src  = -src;
    }

    while (src > 0x00)
    {
        dst[idx++] = (src % 0x0A) + '0';
        src /= 0x0A;
    }

    if (neg)
        dst[idx++] = '-';

    dst[idx] = 0x00;

    for (int j = 0x00; j < idx / 0x02; j++)
    {
        char tmp = dst[j];
        dst[j] = dst[idx - j - 0x01];
        dst[idx - j - 0x01] = tmp;
    }
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

char *strcat(char *dst, const char *src)
{
    char *p = dst;

    while (*p)
    {
        p++;
    }

    while (*src)
    {
        *p++ = *src++;
    }

    *p = 0x00;
    return dst;
}