#include "interpreter.h"
#include "../classfile/attr.h"
#include "../classfile/constant_pool.h"
#include "../runtime/frame.h"
#include "../runtime/operand_stack.h"
#include "../vm/classload.h"
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
                        // 包装一个throw对象
                        object_t *throw_object = calloc(1, sizeof(object_t));
                        cp_info_t *cp_pools = current_frame->current_class->cp_pools;
                        cp_class_t *exception_class = get_cp_class(&cp_pools[exception_table.catch_type]);
                        class_t *exception_class_object = load_class(get_utf8(&cp_pools[exception_class->name_index]), thread);
                        throw_object->class = exception_class_object;
                        clear_operand_stack(current_frame);
                        push(current_frame)->ref = throw_object;
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
        printf("interpret class: %s %s %s\n", 
            thread->current_frame->current_class->class_name,
            thread->current_frame->method->name,
            thread->current_frame->method->descriptor
        );

        exec_instruction(thread);

        if(thread->error != NULL) {
            handle_exception(thread);
        }
    }
}
