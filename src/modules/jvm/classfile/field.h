#pragma once

#include "../utils/bytes.h"
#include "attr.h"

enum field_acc_flags {
    FIELD_ACC_PUBLIC = 0x0001,
    FIELD_ACC_PRIVATE = 0x0002,
    FIELD_ACC_PROTECTED = 0x0004,
    FIELD_ACC_STATIC = 0x0008,
    FIELD_ACC_FINAL = 0x0010,
    FIELD_ACC_VOLATILE = 0x0040,
    FIELD_ACC_TRANSIENT = 0x0080,
    FIELD_ACC_SYNTHETIC = 0x1000,
    FIELD_ACC_ENUM = 0x4000
 };

typedef struct {
    u2 access_flags;
    char *name;
    char *descriptor;
    u2 attributes_count;
    attribute_t *attributes;

    u2 slot_offset_in_class;
    // long or double占两个slot
    u2 slot_count;
    void *init_value;
} field_t;


field_t *read_fields(FILE *file, u2 field_count, cp_info_t *cp_pools);



void field_free(field_t *fields, u2 field_count);