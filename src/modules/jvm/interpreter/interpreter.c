#include "interpreter.h"
#include "../runtime/frame.h"
#include "../runtime/operand_stack.h"
#include "../runtime/class.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_exception(jvm_thread_t *thread) { 

    while(thread->current_frame != NULL) {
        frame_t *current_frame = thread->current_frame;

        method_t *method = current_frame->method;
        if(method->exception_table_length > 0) {
            for(u2 i =0;i<method->exception_table_length;i++) {
                rt_exception_table_t *exception_table = &method->exception_table[i];
                if(current_frame->pc >= exception_table->start_pc && current_frame->pc < exception_table->end_pc && exception_table->end_pc <= method->code->code_length) {
                    // todo 找到handler
                    if(exception_table->handler_pc != 0) {
                        // 包装一个throw对象
                        object_t *throw_object = calloc(1, sizeof(object_t));
                        throw_object->klass = current_frame->current_class->entries[exception_table->catch_type].klass;
                        clear_operand_stack(current_frame);
                        push(current_frame)->ref = throw_object;
                        current_frame->pc = exception_table->handler_pc;
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
        printf("interpret class: %s %s %s\n", 
            thread->current_frame->current_class->class_name,
            thread->current_frame->method->name,
            thread->current_frame->method->descriptor
        );
        if(strcmp("getName", thread->current_frame->method->name) == 0) {
            printf("match method\n");
        }

        exec_instruction(thread);

        if(thread->error != NULL) {
            handle_exception(thread);
        }
    }
}
