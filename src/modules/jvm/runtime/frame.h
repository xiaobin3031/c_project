#pragma once

#include "../classfile/attr.h"
#include "../utils/bytes.h"
#include <stdint.h>

typedef struct frame_t frame_t;

typedef struct {
    uint32_t bits;
    void *ref;
} slot_t;

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

frame_t *frame_new(code_attr_t *codes);




void frame_free(frame_t *frame);