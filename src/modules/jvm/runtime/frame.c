#include "frame.h"
#include "../classfile/attr.h"
#include <stdio.h>
#include <stdlib.h>

frame_t *frame_new(code_attr_t *codes) {
    frame_t *frame = malloc(sizeof(frame_t));
    frame->code = codes->code;
    if(codes->max_locals > 0) {
        frame->local_vars = malloc(codes->max_locals * sizeof(int));
        frame->operand_stack = malloc(codes->max_stack * sizeof(int));
    }
    frame->sp = -1;
    frame->pc = 0;
    return frame;
}





void frame_free(frame_t *frame) {
    if(frame) {
        if(frame->local_vars) free(frame->local_vars);
        if(frame->operand_stack) free(frame->operand_stack);
        free(frame);
    }
}