#pragma once

#include "../utils/bytes.h"
#include "attr.h"

typedef struct {
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
    attribute_t *attributes;

    u2 slot_offset_in_class;
    u2 slot_count;
} field_t;


field_t *read_fields(FILE *file, u2 field_count, cp_info_t *cp_pools);




void field_free(field_t *fields, u2 field_count);