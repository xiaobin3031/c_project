#pragma once

#include "../utils/bytes.h"
#include "../utils/jtype.h"

#include <stdint.h>

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