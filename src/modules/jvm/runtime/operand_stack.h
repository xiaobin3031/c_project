#pragma once

#include "frame.h"
#include "../utils/jtype.h"
#include <ctype.h>

slot_t *pop(frame_t *frame);

slot_t *push(frame_t *frame);

int32_t pop_int(frame_t *frame);

long pop_long(frame_t *frame);

float pop_float(frame_t *frame);

double pop_double(frame_t *frame);

void push_int(frame_t *frame, int32_t v);

void push_long(frame_t *frame, long v);

void push_float(frame_t *frame, float v);

void push_double(frame_t *frame, double v);