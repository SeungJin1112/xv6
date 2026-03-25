#include "types.h"
#include "x86.h"

void *memset(void *dst, int c, uint n)
{
    if ((int)dst % 0x04 == 0x00 && n % 0x04 == 0x00)
    {
        c &= 0xFF;
        stosl(dst, (c << 0x18) | (c << 0x10) | (c << 0x08) | c, n / 0x04);
    }
    else
    {
        stosb(dst, c, n);
    }

    return dst;
}

int memcmp(const void *v1, const void *v2, uint n)
{
    const uchar *s1 = v1;
    const uchar *s2 = v2;

    while (n-- > 0x00)
    {
        if (*s1 != *s2)
            return *s1 - *s2;

        s1++;
        s2++;
    }

    return 0x00;
}

void *memmove(void *dst, const void *src, uint n)
{
    const char *s = src;
    char       *d = dst;

    if (s < d && s + n > d)
    {
        s += n;
        d += n;

        while (n-- > 0x00)
        {
            *--d = *--s;
        }
    }
    else
    {
        while (n-- > 0x00)
        {
            *d++ = *s++;
        }
    }

    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *memcpy(void *dst, const void *src, uint n)
{
    return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n)
{
    while (n > 0x00 && *p && *p == *q)
    {
        n--;
        p++;
        q++;
    }

    if (n == 0x00)
        return 0x00;

    return (uchar)*p - (uchar)*q;
}

char *strncpy(char *s, const char *t, int n)
{
    char *os = s;

    while (n-- > 0x00 && (*s++ = *t++) != 0x00)
        ;

    while (n-- > 0x00)
    {
        *s++ = 0x00;
    }

    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrcpy(char *s, const char *t, int n)
{
    char *os = s;

    if (n <= 0x00)
        return os;

    while (--n > 0x00 && (*s++ = *t++) != 0x00)
        ;

    *s = 0x00;

    return os;
}

int strlen(const char *s)
{
    int n;

    for (n = 0x00; s[n]; n++)
        ;

    return n;
}