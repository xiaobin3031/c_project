#include "interpreter.h"
#include "opcode.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/attr.h"
#include "../runtime/frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static slot_t *pop(frame_t *frame) {
    return frame->operand_stack[frame->sp--];
}
static slot_t *push(frame_t *frame) {
    return frame->operand_stack[++frame->sp];
}
static int pop_int(frame_t *frame) {
    slot_t *slot = pop(frame);
    return slot->i;
}
static long pop_long(frame_t *frame) {
    pop(frame);
    slot_t *slot = pop(frame);
    return slot->l;
}
static float pop_float(frame_t *frame) {
    pop(frame);
    slot_t *slot = pop(frame);
    return slot->f;
}
static double pop_double(frame_t *frame) {
    pop(frame);
    slot_t *slot = pop(frame);
    return slot->d;
}
static void push_int(frame_t *frame, int v) {
    slot_t *slot = push(frame);
    slot->type = SLOAT_TYPE_INT;
    slot->i = v;
}
static void push_long(frame_t *frame, long v) {
    slot_t *slot = push(frame);
    slot->type = SLOAT_TYPE_LONG;
    slot->l = v;
    push(frame)->type = SLOAT_TYPE_NONE;
}
static void push_float(frame_t *frame, float v) {
    slot_t *slot = push(frame);
    slot->type = SLOAT_TYPE_FLOAT;
    slot->f = v;
}
static void push_double(frame_t *frame, double v) {
    slot_t *slot = push(frame);
    slot->type = SLOAT_TYPE_DOUBLE;
    slot->d = v;
    push(frame)->type = SLOAT_TYPE_NONE;
}
static u1 read_code(frame_t *frame) {
    return frame->code[frame->pc++];
}
static void copy_slot(slot_t *target, slot_t *src) {
    target->type = src->type;
    switch(target->type) {
        case SLOAT_TYPE_INT:
            target->i = src->i;
            break;
        case SLOAT_TYPE_FLOAT:
            target->f = src->f;
            break;
        case SLOAT_TYPE_LONG:
            target->l = src->l;
            break;
        case SLOAT_TYPE_DOUBLE:
            target->d = src->d;
            break;
        case SLOAT_TYPE_REF:
            target->ref = src->ref;
            break;
        default:
            fprintf(stderr, "unsupported type: %d\n", target->type);
            exit(1);
    }
}

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
            slot_t *slot = pop(invoker);
            if(slot->type == SLOAT_TYPE_NONE)
                slot = pop(invoker);
            slot_t *local = frame->local_vars[i];
            copy_slot(local, slot);
        }
    }
    return frame;
}

void interpret(frame_t *frame, class_t *class) {
    if(frame->code_length == 0) return;

    u1 opcode;
    void **cp_pools = class->cp_pools;

    while(frame->pc < frame->code_length) {
        opcode = read_code(frame);
        switch(opcode) {
            case OPCODE_iconst_0:
                push_int(frame, 0);
                break;
            case OPCODE_iconst_1:
                push_int(frame, 1);
                break;
            case OPCODE_iconst_2:
                push_int(frame, 2);
                break;
            case OPCODE_iconst_3:
                push_int(frame, 3);
                break;
            case OPCODE_iconst_5:
                push_int(frame, 5);
                break;
            case OPCODE_fconst_1: 
                push_float(frame, 1.0f);
                break;
            case OPCODE_dconst_1:
                push_double(frame, 1.0);
                break;
            case OPCODE_lconst_1:
                push_long(frame, 1L);
                break;
            case OPCODE_istore: {
                u1 index = read_code(frame);
                copy_slot(frame->local_vars[index], pop(frame));
                break;
            }
            case OPCODE_istore_1: {
                copy_slot(frame->local_vars[1], pop(frame));
                break;
            }
            case OPCODE_istore_2: {
                copy_slot(frame->local_vars[2], pop(frame));
                break;
            }
            case OPCODE_istore_3: {
                copy_slot(frame->local_vars[3], pop(frame));
                break;
            }
            case OPCODE_iload: {
                u1 index = read_code(frame);
                push_int(frame, frame->local_vars[index]->i);
                break;
            }
            case OPCODE_iload_0: {
                push_int(frame, frame->local_vars[0]->i);
                break;
            }
            case OPCODE_iload_1:
                push_int(frame, frame->local_vars[1]->i);
                break;
            case OPCODE_iload_2:
                push_int(frame, frame->local_vars[2]->i);
                break;
            case OPCODE_iload_3: 
                push_int(frame, frame->local_vars[3]->i);
                break;
            case OPCODE_iadd: {
                int v2 = pop(frame);
                int v1 = pop(frame);
                int r = v1 + v2;
                push_int(frame, r);
                break;
            }
            case OPCODE_bipush: {
                int bb = read_code(frame);
                push_int(frame, bb);
                break;
            }
            case OPCODE_sipush: {
                u1 v1 = read_code(frame);
                u1 v2 = read_code(frame);
                int r = (v1 << 8) | v2;
                push_int(frame, r);
                break;
            }
            case OPCODE_pop: {
                pop(frame);
                break;
            }
            case OPCODE_iinc: { 
                u1 index = read_code(frame);
                int32_t increment = read_code(frame);
                frame->local_vars[index] += increment;
                break;
            }
            case OPCODE_if_icmpeq: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 == v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_if_icmpne: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 != v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_if_icmplt: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 < v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_if_icmpge: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 >= v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_if_icmpgt: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 > v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_if_icmple: {
                int v2 = pop_int(frame);
                int v1 = pop_int(frame);
                if(v1 <= v2) {
                    u4 store_pc = frame->pc - 1;
                    int8_t bb1 = read_code(frame);
                    int8_t bb2 = read_code(frame);
                    int16_t index = (bb1 << 8) | bb2;
                    frame->pc = store_pc + index;
                }
                break;
            }
            case OPCODE_goto: {
                u4 store_pc = frame->pc - 1;
                int8_t index1 = read_code(frame);
                int8_t index2 = read_code(frame);
                int16_t index = (index1 << 8) | index2;
                frame->pc = store_pc + index;
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
                int arg = pop_int(frame);
                int obj = pop_int(frame);
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
                invoke->operand_stack[++invoke->sp] = pop_int(frame);
                break;
            }
            case OPCODE_return: break;
            case OPCODE_nop: break;
            default: 
                fprintf(stderr, "unknown opcode: %d\n", opcode);
                exit(1);
        }
    }
}