#pragma once

typedef struct {
    char module[32];
    char target[64];
    int port_start;
    int port_end;
} cli_args;

cli_args parse_cli(int argc, char *argv[]);
