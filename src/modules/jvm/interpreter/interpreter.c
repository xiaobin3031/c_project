#include "interpreter.h"
#include "../classfile/attr.h"
#include "../runtime/frame.h"
#include <stdio.h>
#include <stdlib.h>

void handle_exception(jvm_thread_t *thread) { 

    while(thread->current_frame != NULL) {
        frame_t *current_frame = thread->current_frame;

        attr_code_t *attr_code = current_frame->attr_code;
        if(attr_code->exception_table_length > 0) {
            for(u2 i =0;i<attr_code->exception_table_length;i++) {
                exception_table_t exception_table = attr_code->exception_table[i];
                if(current_frame->pc >= exception_table.start_pc && current_frame->pc < exception_table.end_pc && exception_table.end_pc <= attr_code->code_length) {
                    // todo 找到handler
                    if(exception_table.handler_pc != 0) {
                        current_frame->pc = exception_table.handler_pc;
                        // 删除错误信息
                        error_free(thread->error);
                        thread->error = NULL;
                        return;
                    }
                }
            }
        }

        pop_frame(thread);
    }
}

void interpret(jvm_thread_t *thread) {
    while(thread->current_frame != NULL) {

        exec_instruction(thread);

        if(thread->error != NULL) {
            handle_exception(thread);
        }
    }
}
