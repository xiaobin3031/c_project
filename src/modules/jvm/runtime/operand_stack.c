#include "operand_stack.h"
#include "frame.h"
#include "../utils/jtype.h"
#include <ctype.h>
#include <string.h>


slot_t *pop(frame_t *frame) {
    if(frame->sp <= 0) {
        dump_frame(frame);
        perror("stack underflow");
        abort();
    }
    return &frame->operand_stack[--frame->sp];
}

slot_t *push(frame_t *frame) {
    if(frame->sp >= frame->operand_stack_size) {
        dump_frame(frame);
        perror("stack overflow");
        abort();
    }
    return &frame->operand_stack[frame->sp++];
}

int32_t pop_int(frame_t *frame) {
    return (int32_t) pop(frame)->bits;
}

long pop_long(frame_t *frame) {
    uint64_t high = pop(frame)->bits;
    uint64_t low = pop(frame)->bits;
    return (high << 32) | low;
}

float pop_float(frame_t *frame) {
    uint32_t bits = pop(frame)->bits;
    float v;
    memcpy(&v, &bits, sizeof(float));
    return v;
}

double pop_double(frame_t *frame) {
    uint64_t high = pop(frame)->bits;
    uint64_t low = pop(frame)->bits;
    uint64_t bits = (high << 32) | low;

    double v;
    memcpy(&v, &bits, sizeof(double));
    return v;
}

void push_int(frame_t *frame, int32_t v) {
    push(frame)->bits = (uint32_t) v;
}

void push_long(frame_t *frame, long v) {
    uint32_t low = (uint32_t)v;
    uint32_t high = (uint32_t)(v >> 32);
    push(frame)->bits = low;
    push(frame)->bits = high;
}

void push_float(frame_t *frame, float v) {
    slot_t *slot = push(frame);
    memcpy(&slot->bits, &v, sizeof(float));
}

void push_double(frame_t *frame, double v) {
    uint64_t bits;
    memcpy(&bits, &v, sizeof(double));
    push(frame)->bits = (uint32_t)bits;
    push(frame)->bits =  (uint32_t)(bits >> 32);
}