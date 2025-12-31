#pragma once

#include "bytes.h"
#include "../classfile/class_reader.h"

typedef struct slot_t slot_t;

enum atype {
    ATYPE_REF = 1,
    ATYPE_BOOLEAN = 4,
    ATYPE_CHAR = 5,
    ATYPE_FLOAT = 6,
    ATYPE_DOUBLE = 7,
    ATYPE_BYTE = 8,
    ATYPE_SHORT = 9,
    ATYPE_INT = 10,
    ATYPE_LONG = 11
};

typedef struct {
    class_t *class;
    slot_t *fields;

    u1 atype;
    int acount;

    char *strings;
} object_t;

struct slot_t {
    uint32_t bits;
    object_t *ref;
};