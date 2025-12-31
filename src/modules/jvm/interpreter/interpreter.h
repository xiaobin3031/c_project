#pragma once

#include "../runtime/frame.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/constant_pool.h"
#include "../runtime/frame.h"

void handle_exception(jvm_thread_t *thread);

void interpret(jvm_thread_t *thread);

void exec_instruction(jvm_thread_t *thread);

void print_operand_stack(frame_t *frame);

void print_stacktrace(void);

void dump_code_hex(u1 *codes);