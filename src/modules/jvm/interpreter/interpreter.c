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
    if(frame->sp <= 0) {
        dump_frame(frame);
        perror("stack underflow");
        exit(1);
    }
    return &frame->operand_stack[--frame->sp];
}
static slot_t *push(frame_t *frame) {
    if(frame->sp > frame->operand_stack_size) {
        dump_frame(frame);
        perror("stack overflow");
        exit(1);
    }
    return &frame->operand_stack[frame->sp++];
}
static int32_t pop_int(frame_t *frame) {
    return (int32_t) pop(frame)->bits;
}
static long pop_long(frame_t *frame) {
    uint64_t high = pop(frame)->bits;
    uint64_t low = pop(frame)->bits;
    return (high << 32) | low;
}
static float pop_float(frame_t *frame) {
    uint32_t bits = pop(frame)->bits;
    float v;
    memcpy(&v, &bits, sizeof(float));
    return v;
}
static double pop_double(frame_t *frame) {
    uint64_t high = pop(frame)->bits;
    uint64_t low = pop(frame)->bits;
    uint64_t bits = (high << 32) | low;

    double v;
    memcpy(&v, &bits, sizeof(double));
    return v;
}
static void push_int(frame_t *frame, int32_t v) {
    push(frame)->bits = (uint32_t) v;
}
static void push_long(frame_t *frame, long v) {
    uint32_t low = (uint32_t)v;
    uint32_t high = (uint32_t)(v >> 32);
    push(frame)->bits = low;
    push(frame)->bits = high;
}
static void push_float(frame_t *frame, float v) {
    slot_t *slot = push(frame);
    memcpy(&slot->bits, &v, sizeof(float));
}
static void push_double(frame_t *frame, double v) {
    uint64_t bits;
    memcpy(&bits, &v, sizeof(double));
    push(frame)->bits = (uint32_t)bits;
    push(frame)->bits =  (uint32_t)(bits >> 32);
}
static u1 read_code(frame_t *frame) {
    if(frame->pc >= frame->code_length) {
        fprintf(stderr, "pc out of range\n");
        exit(1);
    }
    return frame->code[frame->pc++];
}

static slot_t *get_local(frame_t *frame, u2 index) {
    if(index >= frame->local_var_size) {
        fprintf(stderr, "local var index out of range: %d\n", index);
        exit(1);
    }
    return &frame->local_vars[index];
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
    if(invoker && method->arg_slot_count > 0) {
        for(int i=method->arg_slot_count-1;i>=0;i--) {
            get_local(frame, i)->bits = pop(invoker)->bits;
        }
    }
    return frame;
}

void interpret(frame_t *frame, class_t *class) {
    if(frame->code_length == 0) return;

    u1 opcode;
    void **cp_pools = class->cp_pools;

    while(frame->pc < frame->code_length) {
        opcode = frame->code[frame->pc];
        switch(opcode) {
            case OPCODE_iconst_0: {
                push_int(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_1: {
                push_int(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_2:
                push_int(frame, 2);
                frame->pc++;
                break;
            case OPCODE_iconst_3: {
                push_int(frame, 3);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_5: {
                push_int(frame, 5);
                frame->pc++;
                break;
            }
            case OPCODE_fconst_1:  {
                push_float(frame, 1.0f);
                frame->pc++;
                break;
            }
            case OPCODE_dconst_1: {
                push_double(frame, 1.0);
                frame->pc++;
                break;
            }
            case OPCODE_lconst_1: {
                push_long(frame, 1L);
                frame->pc++;
                break;
            }
            case OPCODE_istore: {
                u1 index = frame->code[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                slot->bits = pop(frame)->bits;
                frame->pc += 2;
                break;
            }
            case OPCODE_istore_1: {
                slot_t *slot = get_local(frame, 1);
                slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_2: {
                slot_t *slot = get_local(frame, 2);
                slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_3: {
                slot_t *slot = get_local(frame, 3);
                slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload: {
                u1 index = frame->code[frame->pc+1];
                push(frame)->bits = get_local(frame, index)->bits;
                frame->pc += 2;
                break;
            }
            case OPCODE_iload_0: {
                push(frame)->bits = get_local(frame, 0)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_1: {
                push(frame)->bits = get_local(frame, 1)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_2: {
                push(frame)->bits = get_local(frame, 2)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_3: {
                push(frame)->bits = get_local(frame, 3)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iadd: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 + v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_bipush: {
                int bb = frame->code[frame->pc+1];
                push_int(frame, bb);
                frame->pc += 2;
                break;
            }
            case OPCODE_sipush: {
                u1 v1 = frame->code[frame->pc+1];
                u1 v2 = frame->code[frame->pc+2];
                int r = (v1 << 8) | v2;
                push_int(frame, r);
                frame->pc += 3;
                break;
            }
            case OPCODE_pop: {
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iinc: { 
                u1 index = frame->code[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                int32_t increment = frame->code[frame->pc+2];
                int32_t value = slot->bits;
                value += increment;

                slot->bits = (uint32_t)value;
                frame->pc += 3;
                break;
            }
            case OPCODE_if_icmpeq: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 == v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+1];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_if_icmpne: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 != v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_if_icmplt: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 < v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_if_icmpge: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 >= v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_if_icmpgt: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 > v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_if_icmple: {
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 <= v2) {
                    u1 bb1 = frame->code[frame->pc+1];
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc++;
                }
                break;
            }
            case OPCODE_goto: {
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                int16_t index = (int16_t)((index1 << 8) | index2);
                frame->pc += index;
                break;
            }
            case OPCODE_getstatic: {
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                printf("[WARN] getstatic #%d ignored.\n", index);
                frame->pc += 3;
                break;
            }
            case OPCODE_invokevirtual: {
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                void *info = cp_pools[index];
                check_cp_info_tag(info, CONSTANT_Methodref);
                cp_methodref_t *methodref = (cp_methodref_t *)info;
                info = cp_pools[methodref->name_and_type_index];
                check_cp_info_tag(info, CONSTANT_NameAndType);
                cp_nameandtype_t *nameandtype = (cp_nameandtype_t *)info;
                char *name = get_utf8(cp_pools[nameandtype->name_index]);
                int32_t arg = pop_int(frame);
                int32_t obj = pop_int(frame);
                // todo 暂时用不到
                (void) obj;
                if(strcmp(name, "println") == 0) {
                    printf("%d\n", arg);
                }
                frame->pc += 3;
                break;
            }
            case OPCODE_invokestatic: {
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                void *info = cp_pools[index];
                method_t *method = find_method(class, info);
                frame_t *sub_frame = create_frame(method, frame);
                sub_frame->invoker = frame;
                interpret(sub_frame, class);
                frame_free(sub_frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_ireturn: {
                frame_t *invoke = frame->invoker;
                push_int(invoke, pop_int(frame));
                frame->pc++;
                break;
            }
            case OPCODE_return: {
                frame->pc++;
                break;
            }
            case OPCODE_nop: {
                frame->pc++;
                break;
            }
            default: 
                fprintf(stderr, "unknown opcode: %d\n", opcode);
                exit(1);
        }
    }
}

void dump_frame(frame_t *frame) {
    printf("[DUMP] frame: \n");
    printf("[DUMP] pc: %d\n", frame->pc);
    printf("[DUMP] sp: %d / %d\n", frame->sp, frame->operand_stack_size);
    printf("[DUMP] opcode: %d\n", frame->code[frame->pc]);
}