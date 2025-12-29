#pragma once

#include "../runtime/frame.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/constant_pool.h"
#include "../runtime/frame.h"

method_t *find_method(class_t *class, cp_info_t *cp_info);

void interpret(frame_t *frame, class_t *class);

void print_operand_stack(frame_t *frame);

void print_stacktrace(void);

void dump_code_hex(u1 *codes);