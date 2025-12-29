#include "frame.h"
#include "../classfile/attr.h"
#include "../utils/jtype.h"
#include "operand_stack.h"
#include "local_vars.h"
#include <stdio.h>
#include <stdlib.h>



frame_t *frame_new(method_t *method, frame_t *invoker) {
    int is_static = method->access_flags & METHOD_ACC_STATIC;
    attr_code_t *code_attr;
    for(u2 i = 0; i < method->attributes_count; i++) {
        if(method->attributes[i].tag == ATTR_CODE) {
            code_attr = (attr_code_t *)method->attributes[i].info;
            break;
        }
    }
    if(code_attr == NULL) {
        fprintf(stderr, "method has no code attribute\n");
        abort();
    }

    u2 max_locals = code_attr->max_locals;
    // 需要存一个this
    if(!is_static) max_locals++;
    size_t frame_size = sizeof(frame_t);

    char *frame_memory = calloc(1, frame_size + max_locals * sizeof(slot_t) + code_attr->max_stack * sizeof(slot_t));
    if(!frame_memory) {
        perror("create frame error by malloc");
        abort();
    }
    frame_t *frame = (frame_t *)frame_memory;
    frame->code = code_attr->code;
    frame->code_length = code_attr->code_length;
    frame->local_var_size = max_locals;
    frame->operand_stack_size = code_attr->max_stack;
    if(max_locals > 0) {
        frame->local_vars = (slot_t*)(frame_memory + frame_size);
    }
    if(code_attr->max_stack > 0) {
        frame->operand_stack = (slot_t*)(frame_memory + frame_size + max_locals * sizeof(slot_t));
    }
    frame->sp = 0;
    frame->pc = 0;
    frame->invoker = invoker;
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
    return frame;
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
    printf("[DUMP] pc: %d / %d\n", frame->pc, frame->code_length);
    printf("[DUMP] sp: %d / %d\n", frame->sp, frame->operand_stack_size);
    printf("[DUMP] opcode: %d\n", frame->code[frame->pc]);
}