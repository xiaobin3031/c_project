#pragma once

#include "../utils/bytes.h"
#include <stdio.h>

typedef struct {
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
    void **attributes;
} method_t;

method_t **read_methods(FILE *file, u2 method_count);