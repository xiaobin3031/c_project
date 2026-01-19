#pragma once

#include "../runtime/frame.h"

void throw_error(jvm_thread_t *thread, enum run_error_e type, const char *message, ...);

class_t *resolve_class(jvm_thread_t *thread, rt_cp_entry_t *entry);

void handle_exception(jvm_thread_t *thread);

void interpret(jvm_thread_t *thread);

void exec_instruction(jvm_thread_t *thread);

void skip_instruction(jvm_thread_t *thread);