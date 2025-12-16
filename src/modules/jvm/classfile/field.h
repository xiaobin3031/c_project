#pragma once

#include "../utils/bytes.h"

typedef struct {
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
    void **attributes;
} field_t;


field_t **read_fields(FILE *file, u2 field_count, void **cp_pools);




void field_free(field_t **fields, u2 field_count);