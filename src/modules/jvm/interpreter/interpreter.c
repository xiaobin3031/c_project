#include "interpreter.h"
#include "opcode.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/attr.h"
#include "../runtime/frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

method_t *find_method(class_t *class, void *info) {
    check_cp_info_tag(info, CONSTANT_Methodref);
    cp_methodref_t *methodref = (cp_methodref_t*)info;
    info = class->cp_pools[methodref->name_and_type_index];
    check_cp_info_tag(info, CONSTANT_NameAndType);
    cp_nameandtype_t *nametype = (cp_nameandtype_t*)info;
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = class->methods[i];
        if(method->name_index == nametype->name_index && method->descriptor_index == nametype->descriptor_index)
            return method;
    }
    char *method_name = get_utf8(class->cp_pools[nametype->name_index]);
    fprintf(stderr, "cannot find method: %s\n", method_name);
    exit(1);
}

frame_t *create_frame(method_t *method, frame_t *invoker) {
    code_attr_t *code;
    for(int i=0;i<method->attributes_count;i++) {
        void *info = method->attributes[i];
        if(is_cp_info_tag(info, ATTR_CODE)) {
            code = (code_attr_t *)info;
            break;
        }
    }
    frame_t *frame = frame_new(code);
    frame->invoker = invoker;
    // 复制方法参数
    if(invoker && method->arg_count > 0) {
        u2 index = method->arg_count;
        if(is_flag(method, METHOD_ACC_STATIC)) {
            index--;
        }
        for(int i=index;i>=0;i--) {
            frame->local_vars[i] = invoker->operand_stack[invoker->sp--];
        }
    }
    return frame;
}

void interpret(frame_t *frame, class_t *class) {
    if(frame->code_length == 0) return;

    u1 opcode;
    void **cp_pools = class->cp_pools;

    while(frame->pc < frame->code_length) {
        opcode = frame->code[frame->pc++];
        switch(opcode) {
            case OPCODE_iconst_1:
                frame->operand_stack[++frame->sp] = 1;
                break;
            case OPCODE_iconst_2:
                frame->operand_stack[++frame->sp] = 2;
                break;
            case OPCODE_istore_1:
                frame->local_vars[1] = frame->operand_stack[frame->sp--];
                break;
            case OPCODE_istore_2:
                frame->local_vars[2] = frame->operand_stack[frame->sp--];
                break;
            case OPCODE_istore_3:
                frame->local_vars[3] = frame->operand_stack[frame->sp--];
                break;
            case OPCODE_iload_0:
                frame->operand_stack[++frame->sp] = frame->local_vars[0];
                break;
            case OPCODE_iload_1:
                frame->operand_stack[++frame->sp] = frame->local_vars[1];
                break;
            case OPCODE_iload_2:
                frame->operand_stack[++frame->sp] = frame->local_vars[2];
                break;
            case OPCODE_iload_3: 
                frame->operand_stack[++frame->sp] = frame->local_vars[3];
                break;
            case OPCODE_iadd: {
                int v2 = frame->operand_stack[frame->sp--];
                int v1 = frame->operand_stack[frame->sp--];
                int r = v1 + v2;
                frame->operand_stack[++frame->sp] = r;
                break;
            }
            case OPCODE_getstatic: {
                u1 index1 = frame->code[frame->pc++];
                u1 index2 = frame->code[frame->pc++];
                u2 index = (index1 << 8) | index2;
                printf("[WARN] getstatic #%d ignored.\n", index);
                break;
            }
            case OPCODE_invokevirtual: {
                u1 index1 = frame->code[frame->pc++];
                u1 index2 = frame->code[frame->pc++];
                u2 index = (index1 << 8) | index2;
                void *info = cp_pools[index];
                check_cp_info_tag(info, CONSTANT_Methodref);
                cp_methodref_t *methodref = (cp_methodref_t *)info;
                info = cp_pools[methodref->name_and_type_index];
                check_cp_info_tag(info, CONSTANT_NameAndType);
                cp_nameandtype_t *nameandtype = (cp_nameandtype_t *)info;
                char *name = get_utf8(cp_pools[nameandtype->name_index]);
                printf("method_name: %s\n", name);
                int arg = frame->operand_stack[frame->sp--];
                int obj = frame->operand_stack[frame->sp--];
                // todo 暂时用不到
                (void) obj;
                if(strcmp(name, "println") == 0) {
                    printf("%d\n", arg);
                }
                break;
            }
            case OPCODE_invokestatic: {
                u1 index1 = frame->code[frame->pc++];
                u1 index2 = frame->code[frame->pc++];
                u2 index = (index1 << 8) | index2;
                void *info = cp_pools[index];
                method_t *method = find_method(class, info);
                frame_t *sub_frame = create_frame(method, frame);
                sub_frame->invoker = frame;
                interpret(sub_frame, class);
                frame_free(sub_frame);
                break;
            }
            case OPCODE_ireturn: {
                frame_t *invoke = frame->invoker;
                invoke->operand_stack[++invoke->sp] = frame->operand_stack[frame->sp--];
                break;
            }
            case OPCODE_return:
                break;
            default: 
                fprintf(stderr, "unknown oframe->pcode: %d\n", opcode);
                exit(1);
        }
    }
}