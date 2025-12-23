#pragma once

#include "../classfile/class_reader.h"

typedef struct {
    class_t *class;
    slot_t *fields;
} object_t;