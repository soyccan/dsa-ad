#include <stdio.h>
#include <string.h>

#include "common.h"
#include "query.h"


int main()
{
    char cmd[10];
    char arg[3][33];
    int iarg;
    float farg;
    while (1) {
        scanf("%9s", cmd);
        if (strncmp(cmd, "get", sizeof cmd) == 0) {
            scanf("%32s %32s %d", arg[0], arg[1], &iarg);
            puts("********************");
            get(arg[0], arg[1], iarg);
            puts("********************");
        } else if (strncmp(cmd, "purchased", sizeof cmd) == 0) {
            scanf("%32s", arg[0]);
            puts("********************");
            purchased(arg[0]);
            puts("********************");
        } else if (strncmp(cmd, "clicked", sizeof cmd) == 0) {
            scanf("%32s %32s", arg[0], arg[1]);
            puts("********************");
            clicked(arg[0], arg[1]);
            puts("********************");
        } else if (strncmp(cmd, "profit", sizeof cmd) == 0) {
            scanf("%d %f", &iarg, &farg);
            puts("********************");
            profit(iarg, farg);
            puts("********************");
        } else if (strncmp(cmd, "quit", sizeof cmd) == 0) {
            break;
        } else {
            printf("unrecognized command: %s\n", cmd);
        }
    }
    return 0;
}