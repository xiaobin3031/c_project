#include "frame.h"
#include "../classfile/attr.h"
#include "../utils/jtype.h"
#include <stdio.h>
#include <stdlib.h>

frame_t *frame_new(attribute_t *codes, int is_static) {
    u2 max_stack = parse_to_u2(codes->info);
    u2 max_locals = parse_to_u2(codes->info + 2);
    // 需要存一个this
    if(!is_static) max_locals++;
    u4 code_length = parse_to_u4(codes->info + 4);
    size_t frame_size = sizeof(frame_t);

    char *frame_memory = calloc(1, frame_size + max_locals * sizeof(slot_t) + max_stack * sizeof(slot_t));
    if(!frame_memory) {
        perror("create frame error by malloc");
        abort();
    }
    frame_t *frame = (frame_t *)frame_memory;
    frame->code = codes->info + 8;
    frame->code_length = code_length;
    frame->local_var_size = max_locals;
    frame->operand_stack_size = max_stack;
    if(max_locals > 0) {
        frame->local_vars = (slot_t*)(frame_memory + frame_size);
    }
    if(max_stack > 0) {
        frame->operand_stack = (slot_t*)(frame_memory + frame_size + max_locals * sizeof(slot_t));
    }
    frame->sp = 0;
    frame->pc = 0;
    frame->invoker = NULL;
    return frame;
}




void frame_free(frame_t *frame) {
    // if(frame) {
    //     if(frame->local_vars) free(frame->local_vars);
    //     if(frame->operand_stack) free(frame->operand_stack);
    //     free(frame);
    // }
}