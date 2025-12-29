#pragma once

#include "../utils/bytes.h"
#include "../utils/jtype.h"
#include "../classfile/method_info.h"

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

frame_t *frame_new(method_t *method, frame_t *invoker);




void frame_free(frame_t *frame);

void dump_frame(frame_t *frame);