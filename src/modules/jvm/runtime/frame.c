#include "frame.h"
#include "operand_stack.h"
#include "local_vars.h"
#include "class.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

error_t *error_new(enum run_error_e type, const char *message) {
    error_t *error = malloc(sizeof(error_t));
    error->type = type;
    error->message = NULL;
    if(message && *message) {
        error->message = strdup(message);
    }
    return error;
}

void pop_frame(jvm_thread_t *thread) {
    frame_t *current_frame = thread->current_frame;
    frame_t *frame = current_frame->invoker;
    thread->current_frame = frame;
    frame_free(current_frame);
}

void push_frame(jvm_thread_t *thread, frame_t *frame) {
    frame_t *current_frame = thread->current_frame;
    frame->invoker = current_frame;
    thread->current_frame = frame;
}

frame_t *frame_new(method_t *method, frame_t *invoker) {
    int is_static = method->access_flags & METHOD_ACC_STATIC;
    frame_t *frame = NULL;
    u2 max_locals = 0, max_stacks = 0;

    if(method->access_flags & METHOD_ACC_NATIVE) {
        // native 方法，直接指定参数
        max_locals = method->arg_slot_count;
        max_stacks = method->return_slot_count;
    } else {
        // 非native方法，从code attr中获取执行的代码
        if(method->code == NULL) {
            fprintf(stderr, "method has no code attribute\n");
            abort();
        }
        max_locals = method->code->max_locals;
        max_stacks = method->code->max_stack;
    }
    // 需要存一个this，jvm规定
    if(!is_static) max_locals++;

    size_t frame_size = sizeof(frame_t);
    char *frame_memory = calloc(1, frame_size + max_locals * sizeof(slot_t) + max_stacks * sizeof(slot_t));
    if(!frame_memory) {
        perror("create frame error by malloc");
        abort();
    }

    frame = (frame_t *)frame_memory;

    frame->local_var_size = max_locals;
    frame->operand_stack_size = max_stacks;
    if(max_locals > 0) {
        frame->local_vars = (slot_t*)(frame_memory + frame_size);
    }
    if(max_stacks > 0) {
        frame->operand_stack = (slot_t*)(frame_memory + frame_size + max_locals * sizeof(slot_t));
    }
    frame->sp = 0;
    frame->pc = 0;
    // 复制方法参数
    if(invoker) {
        u2 total_slot_count = method->arg_slot_count;
        if(!is_static) total_slot_count++;

        // 2️⃣ 再处理参数（arg_slot_count 个 slot）
        for (int i = total_slot_count - 1; i >= 0; i--) {
            slot_t *local = get_local(frame, i);
            slot_t *stack = pop(invoker);
            local->bits = stack->bits;
            local->ref = stack->ref;
        }
    }
    frame->method = method;
    return frame;
}

jvm_thread_t *jvm_thread_new() {
    jvm_thread_t *thread = malloc(sizeof(jvm_thread_t));
    thread->current_frame = NULL;
    thread->error = NULL;
    return thread;
}



void frame_free(frame_t *frame) {
    // if(frame) {
    //     if(frame->local_vars) free(frame->local_vars);
    //     if(frame->operand_stack) free(frame->operand_stack);
    //     free(frame);
    // }
}

void dump_frame(frame_t *frame) {
    printf("[DUMP] frame: \n");
    printf("[DUMP] local vars: %d\n", frame->local_var_size);
    printf("[DUMP] sp: %d / %d\n", frame->sp, frame->operand_stack_size);
    if(frame->method->code != NULL) {
        printf("[DUMP] opcode: %d\n", frame->method->code->codes[frame->pc]);
        printf("[DUMP] pc: %d / %d\n", frame->pc, frame->method->code->code_length);
    }
}

void error_free(error_t *error) {
    if(error) {
        if(error->message) free(error->message);
        free(error);
    }
}