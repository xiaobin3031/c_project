#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cli_args parse_cli(int argc, char *argv[])
{
    cli_args args;
    memset(&args, 0, sizeof(args));

    if (argc < 3) {
        printf("Usage: shentou <module> <target> --ports 1-1000\n");
        exit(1);
    }

    strcpy(args.module, argv[1]);
    strcpy(args.target, argv[2]);

    // 解析端口范围
    args.port_start = 1;
    args.port_end  = 65535;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--ports") == 0 && i + 1 < argc) {
            int start, end;
            sscanf(argv[i+1], "%d-%d", &start, &end);
            args.port_start = start;
            args.port_end   = end;
        }else if(strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            args.port_start = args.port_end = atoi(argv[i+1]);
        }
    }

    return args;
}
