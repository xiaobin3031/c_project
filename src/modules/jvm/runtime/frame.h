#pragma once

#include "../utils/bytes.h"
#include "../utils/jtype.h"
#include "../classfile/method_info.h"
#include "../classfile/attr.h"
#include "run_error.h"

#include <stdint.h>

typedef struct jvm_thread_t jvm_thread_t;
typedef struct frame_t frame_t;

typedef struct {
    enum run_error_e type;
    char *message;
} error_t;

struct frame_t {
    slot_t *local_vars;
    slot_t *operand_stack;
    u2 local_var_size;
    u2 operand_stack_size;
    int16_t sp;
    u4 pc;

    attr_code_t *attr_code;

    class_t *current_class;
    frame_t *invoker;
    jvm_thread_t *thread;
};

struct jvm_thread_t {
    frame_t *current_frame;

    error_t *error;
};

void pop_frame(jvm_thread_t *thread);

void push_frame(jvm_thread_t *thread, frame_t *frame);

frame_t *frame_new(method_t *method, frame_t *invoker, class_t *current_class);

jvm_thread_t *jvm_thread_new();

void frame_free(frame_t *frame);

void dump_frame(frame_t *frame);