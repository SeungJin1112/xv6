#include "types.h"
#include "stat.h"
#include "user.h"

void cat(int fd)
{
    char buffer[512] = { 0x00 };

    int bytes = 0x00;

    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0x00)
    {
        if (write(0x01, buffer, bytes) != bytes)
        {
            printf(0x02, "[LVL: ERROR][PROC: CAT][FUNC: %s][LINE: %d] write error\n", __func__, __LINE__);
            exit();
        }
    }

    if (bytes < 0x00)
    {
        printf(0x02, "[LVL: ERROR][PROC: CAT][FUNC: %s][LINE: %d] read error\n", __func__, __LINE__);
        exit();
    }
}

int main(int argc, char *argv[])
{
    int fd = -0x01;

    if (argc <= 0x01)
    {
        // cat(0x00);
        exit();
    }

    for (int i = 0x01; i < argc; i++)
    {
        if ((fd = open(argv[i], 0x00)) < 0x00)
        {
            printf(0x02, "[LVL: ERROR][PROC: CAT][FUNC: %s][LINE: %d] cannot open %s\n", __func__, __LINE__, argv[i]);
            exit();
        }

        cat(fd);
        close(fd);
    }

    exit();
}
