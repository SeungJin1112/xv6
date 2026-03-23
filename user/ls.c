#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char *fmtname(char *path)
{
    static char buffer[DIRSIZ + 0x01] = { 0x00 };

    char *p = nullptr;

    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;

    p++;

    if (strlen(p) >= DIRSIZ)
        return p;

    memmove(buffer, p, strlen(p));
    memset(buffer + strlen(p), ' ', DIRSIZ - strlen(p));

    return buffer;
}

void ls(char *path)
{
    char buffer[512] = { 0x00 };

    char *p = nullptr;

    int fd  = -0x01;

    struct dirent de = { 0x00 };
    struct stat st   = { 0x00 };

    if ((fd = open(path, 0x00)) < 0x00)
    {
        printf(0x02, "[LVL: ERROR][PROC: LS][FUNC: %s][LINE: %d] cannot open %s\n", __func__, __LINE__, path);
        return;
    }

    if (fstat(fd, &st) < 0x00)
    {
        printf(0x02, "[LVL: ERROR][PROC: LS][FUNC: %s][LINE: %d] cannot stat %s\n", __func__, __LINE__, path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
        printf(0x01, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
        break;

    case T_DIR:
        if (strlen(path) + 0x01 + DIRSIZ + 0x01 > sizeof buffer)
        {
            printf(0x02, "[LVL: WARN][PROC: LS][FUNC: %s][LINE: %d] path too long: %s\n", __func__, __LINE__, path);
            break;
        }

        strcpy(buffer, path);

        p = buffer + strlen(buffer);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0x00)
                continue;

            memmove(p, de.name, DIRSIZ);

            p[DIRSIZ] = 0x00;

            if (stat(buffer, &st) < 0x00)
            {
                printf(0x02, "[LVL: ERROR][PROC: LS][FUNC: %s][LINE: %d] cannot stat %s\n", __func__, __LINE__, buffer);
                continue;
            }

            printf(0x01, "%s %d %d %d\n", fmtname(buffer), st.type, st.ino, st.size);
        }

        break;
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    int i = 0x00;

    if (argc < 0x02)
    {
        ls(".");
        exit();
    }

    for (i = 0x01; i < argc; i++)
    {
        ls(argv[i]);
    }

    exit();
}
