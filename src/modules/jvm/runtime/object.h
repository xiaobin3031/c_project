#pragma once

#include "../classfile/class_reader.h"

typedef struct {
    class_t *class;
    void *fields;
} object_t;