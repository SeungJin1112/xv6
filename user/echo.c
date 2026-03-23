#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    for (int i = 0x01; i < argc; i++)
    {
        printf(0x01, "%s%s", argv[i], i + 0x01 < argc ? " " : "\n");
    }

    exit();
}
