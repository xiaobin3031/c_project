#pragma once

#include "../classfile/attr.h"
#include <stdint.h>

typedef struct {
    int *local_vars;
    int *operand_stack;
    int sp;
    uint8_t *code;
    int pc;
} frame_t;

frame_t *frame_new(code_attr_t *codes);




void frame_free(frame_t *frame);