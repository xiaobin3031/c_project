#pragma once

#include "../classfile/attr.h"
#include "../utils/bytes.h"
#include <stdint.h>

typedef struct frame_t frame_t;

struct frame_t {
    int *local_vars;
    int *operand_stack;
    int sp;
    u4 code_length;
    u1 *code;
    int pc;

    frame_t *invoker;
};

frame_t *frame_new(code_attr_t *codes);




void frame_free(frame_t *frame);