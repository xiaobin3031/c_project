#pragma once

#include "../classfile/attr.h"
#include "../utils/bytes.h"
#include <stdint.h>

typedef enum {
    // 占位使用
    SLOAT_TYPE_NONE = 0,

    SLOAT_TYPE_INT = 1,
    SLOAT_TYPE_FLOAT = 2,
    SLOAT_TYPE_LONG = 3,
    SLOAT_TYPE_DOUBLE = 4,
    SLOAT_TYPE_REF = 5
} slot_type;

typedef struct frame_t frame_t;

typedef struct {
    u1 type;
    union {
        int32_t i;
        float f;
        int64_t l;
        double d;
        void *ref;
    };
} slot_t;

struct frame_t {
    slot_t **local_vars;
    slot_t **operand_stack;
    u2 local_var_size;
    u2 operand_stack_size;
    u2 sp;
    u4 code_length;
    u1 *code;
    u4 pc;

    frame_t *invoker;
};

frame_t *frame_new(code_attr_t *codes);




void frame_free(frame_t *frame);