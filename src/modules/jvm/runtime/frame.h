#pragma once

#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../utils/bytes.h"

#include <stdint.h>

typedef struct frame_t frame_t;
typedef struct slot_t slot_t;

typedef struct {
    class_t *class;
    slot_t *fields;
} object_t;

struct slot_t {
    uint32_t bits;
    object_t *ref;
};

struct frame_t {
    slot_t *local_vars;
    slot_t *operand_stack;
    u2 local_var_size;
    u2 operand_stack_size;
    int16_t sp;
    u4 code_length;
    u1 *code;
    u4 pc;

    frame_t *invoker;
};

frame_t *frame_new(attribute_t *codes, int is_static);




void frame_free(frame_t *frame);