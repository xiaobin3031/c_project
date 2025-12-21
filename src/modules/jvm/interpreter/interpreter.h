#pragma once

#include "../runtime/frame.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"

method_t *find_method(class_t *class, void *cp_info);

frame_t *create_frame(method_t *method, frame_t *invoker);

void interpret(frame_t *frame, class_t *class);

void dump_frame(frame_t *frame);