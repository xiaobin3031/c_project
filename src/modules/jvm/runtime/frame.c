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
            frame->local_vars = malloc(codes->max_locals * sizeof(slot_t *));
            for(u2 i = 0; i < codes->max_locals; i++){
                frame->local_vars[i] = malloc(sizeof(slot_t));
            }
        }
        if(codes->max_stack > 0) {
            frame->operand_stack = malloc(codes->max_stack * sizeof(slot_t *));
            for(u2 i = 0; i < codes->max_stack; i++){
                frame->operand_stack[i] = malloc(sizeof(slot_t));
            }
        }
    }else{
        frame->code_length = 0;
    }
    frame->sp = -1;
    frame->pc = 0;
    return frame;
}




static void free_slot(slot_t **slot, u2 length) {
    if(slot) {
        for(u2 i = 0; i < length; i++) {
            free(slot[i]);
        }
        free(slot);
    }
}
void frame_free(frame_t *frame) {
    if(frame) {
        free_slot(frame->local_vars, frame->local_var_size);
        free_slot(frame->operand_stack, frame->operand_stack_size);
        free(frame);
    }
}