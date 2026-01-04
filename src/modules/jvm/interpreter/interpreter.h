#pragma once

#include "../runtime/frame.h"

void handle_exception(jvm_thread_t *thread);

void interpret(jvm_thread_t *thread);

void exec_instruction(jvm_thread_t *thread);