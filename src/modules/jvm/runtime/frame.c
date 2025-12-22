#include "frame.h"
#include "../classfile/attr.h"
#include <stdio.h>
#include <stdlib.h>

frame_t *frame_new(code_attr_t *codes) {
    frame_t *frame = malloc(sizeof(frame_t));
    if(!frame) {
        perror("create frame error by malloc");
        exit(1);
    }
    if(codes) {
        frame->code = codes->code;
        frame->code_length = codes->code_length;
        frame->local_var_size = codes->max_locals;
        frame->operand_stack_size = codes->max_stack;
        if(codes->max_locals > 0) {
            frame->local_vars = calloc(codes->max_locals, sizeof(slot_t));
            if(!frame->local_vars) {
                perror("create frame local vars error by calloc");
                abort();
            }
        }
        if(codes->max_stack > 0) {
            frame->operand_stack = calloc(codes->max_stack, sizeof(slot_t));
            if(!frame->operand_stack) {
                perror("create frame operand stack error by calloc");
                abort();
            }
        }
    }else{
        frame->code_length = 0;
    }
    frame->sp = 0;
    frame->pc = 0;
    frame->invoker = NULL;
    return frame;
}




void frame_free(frame_t *frame) {
    if(frame) {
        if(frame->local_vars) free(frame->local_vars);
        if(frame->operand_stack) free(frame->operand_stack);
        free(frame);
    }
}