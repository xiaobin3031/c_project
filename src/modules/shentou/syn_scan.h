#pragma once

#include "../../core/cli.h"
#include <stdint.h>

typedef struct {
    uint16_t sport;
    uint16_t dport;
} scan_packet ;

#define MAX_THREADS 100

void syn_scan(cli_args *args);
