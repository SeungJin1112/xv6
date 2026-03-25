// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = {"sh", 0x00};

int main(void)
{
    int pid  = -0x01;
    int wpid = -0x01;

    if (open("console", O_RDWR) < 0x00)
    {
        mknod("console", 0x01, 0x01);
        open("console", O_RDWR);
    }

    dup(0x00); // stdout
    dup(0x00); // stderr

    while (true)
    {
        printf(0x01, "[LVL: INFO][PROC: INIT][FUNC: %s][LINE: %d] starting sh\n", __func__, __LINE__);

        pid = fork();

        if (pid < 0x00)
        {
            printf(0x02, "[LVL: ERROR][PROC: INIT][FUNC: %s][LINE: %d] fork failed\n", __func__, __LINE__);
            exit();
        }

        if (pid == 0x00)
        {
            exec("sh", argv);
            printf(0x02, "[LVL: ERROR][PROC: INIT][FUNC: %s][LINE: %d] exec sh failed\n", __func__, __LINE__);
            exit();
        }

        while ((wpid = wait()) >= 0x00 && wpid != pid)
            ;
    }
}
