#include "interpreter.h"
#include "opcode.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/constant_pool.h"
#include "../classfile/attr.h"
#include "../runtime/frame.h"
#include "../vm/vm.h"
#include "../utils/slots.h"
#include "../utils/jtype.h"
#include "../native/string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>

static slot_t *pop(frame_t *frame) {
    if(frame->sp <= 0) {
        dump_frame(frame);
        perror("stack underflow");
        abort();
    }
    return &frame->operand_stack[--frame->sp];
}
static slot_t *push(frame_t *frame) {
    if(frame->sp >= frame->operand_stack_size) {
        dump_frame(frame);
        perror("stack overflow");
        abort();
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
        abort();
    }
    return frame->code[frame->pc++];
}

static slot_t *get_local(frame_t *frame, u2 index) {
    if(index >= frame->local_var_size) {
        fprintf(stderr, "local var index out of range: %d\n", index);
        dump_frame(frame);
        abort();
    }
    return &frame->local_vars[index];
}

method_t *find_method(class_t *class, cp_info_t *info) {
    check_cp_info_tag(info->tag, CONSTANT_Methodref);
    u2 index = (info->info[2] << 8) | info->info[3];
    cp_info_t nametype = class->cp_pools[index];
    check_cp_info_tag(nametype.tag, CONSTANT_NameAndType);
    u2 name_index = (nametype.info[0] << 8) | nametype.info[1];
    u2 descriptor_index = (nametype.info[2] << 8) | nametype.info[3];
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = &class->methods[i];
        if(method->name_index == name_index && method->descriptor_index == descriptor_index)
            return method;
    }
    char *method_name = get_utf8(&class->cp_pools[name_index]);
    fprintf(stderr, "cannot find method: %s\n", method_name);
    abort();
}

frame_t *create_frame(method_t *method, frame_t *invoker) {
    attribute_t *code;
    for(int i=0;i<method->attributes_count;i++) {
        attribute_t attr = method->attributes[i];
        if(is_cp_info_tag(attr.tag, ATTR_CODE)) {
            code = &attr;
            break;
        }
    }
    if(!code) {
        perror("method code not found");
        abort();
    }
    int is_static = method_is_flag(method->access_flags, METHOD_ACC_STATIC);
    frame_t *frame = frame_new(code, is_static);
    frame->invoker = invoker;
    // 复制方法参数
    if(invoker) {
        u2 total_slot_count = method->arg_slot_count;
        printf("total_slot_count: %d\n", total_slot_count);
        if(!is_static) total_slot_count++;
        printf("total_slot_count 2: %d\n", total_slot_count);

        // 2️⃣ 再处理参数（arg_slot_count 个 slot）
        for (int i = total_slot_count - 1; i >= 0; i--) {
            slot_t *local = get_local(frame, i);
            slot_t *stack = pop(invoker);
            local->bits = stack->bits;
            local->ref = stack->ref;
        }
    }
    return frame;
}

static void go_to_by_index(frame_t *frame) {
    u1 bb1 = frame->code[frame->pc+1];
    u1 bb2 = frame->code[frame->pc+2];
    int16_t index = (int16_t)((bb1 << 8) | bb2);
    frame->pc += index;
}

void interpret(frame_t *frame, class_t *class) {
    if(frame->code_length == 0) return;

    u1 opcode;
    cp_info_t *cp_pools = class->cp_pools;

    while(frame->pc < frame->code_length) {
        opcode = frame->code[frame->pc];
        printf("opcode: %d\n", opcode);
        switch(opcode) {
            // Constants
            case OPCODE_nop: {   // 0x00,  // 00 
                frame->pc++;
                break;
            }
            case OPCODE_aconst_null: {   // 0x01,  // 01 
                fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                abort();
            }
            case OPCODE_iconst_m1: {   // 0x02,  // 02 
                push_int(frame, -1);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_0: {   // 0x03,  // 03 
                push_int(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_1: {   // 0x04,  // 04 
                push_int(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_2: {   // 0x05,  // 05 
                push_int(frame, 2);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_3: {   // 0x06,  // 06 
                push_int(frame, 3);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_4: {   // 0x07,  // 07 
                push_int(frame, 4);
                frame->pc++;
                break;
            }
            case OPCODE_iconst_5: {   // 0x08,  // 08 
                push_int(frame, 5);
                frame->pc++;
                break;
            }
            case OPCODE_lconst_0: {   // 0x09,  // 09 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lconst_1: {   // 0x0a,  // 10 
                push_long(frame, 1L);
                frame->pc++;
                break;
            }
            case OPCODE_fconst_0: {   // 0x0b,  // 11 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fconst_1: {   // 0x0c,  // 12 
                push_float(frame, 1.0f);
                frame->pc++;
                break;
            }
            case OPCODE_fconst_2: {   // 0x0d,  // 13 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dconst_0: {   // 0x0e,  // 14 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dconst_1: {   // 0x0f,  // 15 
                push_double(frame, 1.0);
                frame->pc++;
                break;
            }
            case OPCODE_bipush: {   // 0x10,  // 16 
                int32_t bb = (int32_t)frame->code[frame->pc+1];
                push(frame)->bits = (uint32_t)bb;
                frame->pc += 2;
                break;
            }
            case OPCODE_sipush: {   // 0x11,  // 17 
                u1 v1 = frame->code[frame->pc+1];
                u1 v2 = frame->code[frame->pc+2];
                u2 r = (v1 << 8) | v2;
                push_int(frame, (int32_t)r);
                frame->pc += 3;
                break;
            }
            case OPCODE_ldc: {   // 0x12,  // 18 
                u1 index = frame->code[frame->pc+1];
                cp_info_t cp_info = cp_pools[index];
                if(is_cp_info_tag(cp_info.tag, CONSTANT_Integer)) {
                    push_int(frame, (int32_t)parse_to_u4(cp_info.info));
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_Float)) {
                    push(frame)->bits = parse_to_u4(cp_info.info);
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_String)) {
                    cp_info_t cp_string = cp_pools[index];
                    char *string = get_utf8(&cp_pools[parse_to_u2(cp_string.info)]);
                    object_t *ref = new_string_object(string);
                    push(frame)->ref = ref;
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_Class)) {
                    fprintf(stderr, "ldc class not implemented\n");
                    abort();
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_MethodType)) {
                    fprintf(stderr, "ldc method type not implemented\n");
                    abort();
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_MethodHandle)) {
                    fprintf(stderr, "ldc method handle not implemented\n");
                    abort();
                }else if(is_cp_info_tag(cp_info.tag, CONSTANT_InvokeDynamic)) {
                    fprintf(stderr, "ldc invoke dynamic not implemented\n");
                    abort();
                }
                frame->pc += 2;
                break;
            }
            case OPCODE_ldc_w: {   // 0x13,  // 19 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ldc2_w: {   // 0x14,  // 20 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Loads
            case OPCODE_iload: {   // 0x15,     // 21 
                u1 index = frame->code[frame->pc+1];
                push(frame)->bits = get_local(frame, index)->bits;
                frame->pc += 2;
                break;
            }
            case OPCODE_lload: {   // 0x16,     // 22 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fload: {   // 0x17,     // 23 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dload: {   // 0x18,     // 24 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_aload: {   // 0x19,     // 25 
                u1 index = frame->code[frame->pc + 1];
                push(frame)->ref = get_local(frame, index)->ref;
                frame->pc += 2;
                break;
            }
            case OPCODE_iload_0: {   // 0x1a,     // 26 
                push(frame)->bits = get_local(frame, 0)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_1: {   // 0x1b,     // 27 
                push(frame)->bits = get_local(frame, 1)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_2: {   // 0x1c,     // 28 
                push(frame)->bits = get_local(frame, 2)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_iload_3: {   // 0x1d,     // 29 
                push(frame)->bits = get_local(frame, 3)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_lload_0: {   // 0x1e,     // 30 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lload_1: {   // 0x1f,     // 31 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lload_2: {   // 0x20,     // 32 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lload_3: {   // 0x21,     // 33 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fload_0: {   // 0x22,     // 34 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fload_1: {   // 0x23,     // 35 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fload_2: {   // 0x24,     // 36 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fload_3: {   // 0x25,     // 37 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dload_0: {   // 0x26,     // 38 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dload_1: {   // 0x27,     // 39 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dload_2: {   // 0x28,     // 40 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dload_3: {   // 0x29,     // 41 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_aload_0: {   // 0x2a,     // 42 
                push(frame)->ref = get_local(frame, 0)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_1: {   // 0x2b,     // 43 
                push(frame)->ref = get_local(frame, 1)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_2: {   // 0x2c,     // 44 
                push(frame)->ref = get_local(frame, 2)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_3: {   // 0x2d,     // 45 
                push(frame)->ref = get_local(frame, 3)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_iaload: {   // 0x2e,     // 46 
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                if(index < 0 || index >= array->acount) {
                    // todo throw exception
                    abort();
                }
                slot_t slot = array->fields[index];
                push(frame)->bits = slot.bits;
                frame->pc++;
                break;
            }
            case OPCODE_laload: {   // 0x2f,     // 47 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_faload: {   // 0x30,     // 48 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_daload: {   // 0x31,     // 49 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_aaload: {   // 0x32,     // 50 
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                slot_t *slot = push(frame);
                slot->ref = array->fields[index].ref;
                frame->pc++;
                break;
            }
            case OPCODE_baload: {   // 0x33,     // 51 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_caload: {   // 0x34,     // 52 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_saload: {   // 0x35,     // 53 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Stores
            case OPCODE_istore: {   // 0x36,       // 54 
                u1 index = frame->code[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                slot->bits = pop(frame)->bits;
                frame->pc += 2;
                break;
            }
            case OPCODE_lstore: {   // 0x37,       // 55 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fstore: {   // 0x38,       // 56 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dstore: {   // 0x39,       // 57 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_astore: {   // 0x3a,       // 58 
                u1 index = frame->code[frame->pc + 1];
                get_local(frame, index)->ref = pop(frame)->ref;
                frame->pc += 2;
                break;
            }
            case OPCODE_istore_0: {   // 0x3b,       // 59 
                slot_t *local_slot = get_local(frame, 0);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_1: {   // 0x3c,       // 60 
                slot_t *local_slot = get_local(frame, 1);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_2: {   // 0x3d,       // 61 
                slot_t *local_slot = get_local(frame, 2);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_3: {   // 0x3e,       // 62 
                slot_t *local_slot = get_local(frame, 3);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_lstore_0: {   // 0x3f,       // 63 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lstore_1: {   // 0x40,       // 64 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lstore_2: {   // 0x41,       // 65 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lstore_3: {   // 0x42,       // 66 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fstore_0: {   // 0x43,       // 67 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fstore_1: {   // 0x44,       // 68 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fstore_2: {   // 0x45,       // 69 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fstore_3: {   // 0x46,       // 70 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dstore_0: {   // 0x47,       // 71 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dstore_1: {   // 0x48,       // 72 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dstore_2: {   // 0x49,       // 73 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dstore_3: {   // 0x4a,       // 74 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_astore_0: {   // 0x4b,       // 75 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_astore_1: {   // 0x4c,       // 76 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_astore_2: {   // 0x4d,       // 77 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_astore_3: {   // 0x4e,       // 78 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iastore: {   // 0x4f,       // 79 
                int32_t value = pop_int(frame);
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                if(array->atype != ATYPE_INT) {
                    // TODO: throw exception
                    abort();
                }
                if(index >= array->acount) {
                    // TODO: throw exception
                    abort();
                }
                slot_t slot = array->fields[index];
                slot.bits = (uint32_t)value;
                frame->pc++;
                break;
            }
            case OPCODE_lastore: {   // 0x50,       // 80 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fastore: {   // 0x51,       // 81 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dastore: {   // 0x52,       // 82 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_aastore: {   // 0x53,       // 83 
                object_t *value = pop(frame)->ref;
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                array->fields[index].ref = value;
                frame->pc++;
                break;
            }
            case OPCODE_bastore: {   // 0x54,       // 84 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_castore: {   // 0x55,       // 85 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_sastore: {   // 0x56,       // 86
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Stack
            case OPCODE_pop: {   // 0x57,      // 87 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_pop2: {   // 0x58,      // 88 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dup: {   // 0x59,      // 89 
                slot_t *slot = pop(frame);
                push(frame);
                slot_t *dup_slot = push(frame);
                dup_slot->bits = slot->bits;
                dup_slot->ref = slot->ref;
                frame->pc++;
                break;
            }
            case OPCODE_dup_x1: {   // 0x5a,      // 90 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dup_x2: {   // 0x5b,      // 91 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dup2: {   // 0x5c,      // 92 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dup2_x1: {   // 0x5d,      // 93 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dup2_x2: {   // 0x5e,      // 94 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_swap: {   // 0x5f,      // 95 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Math
            case OPCODE_iadd: {   // 0x60,       // 96 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 + v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_ladd: {   // 0x61,       // 97 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fadd: {   // 0x62,       // 98 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dadd: {   // 0x63,       // 99 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_isub: {   // 0x64,       // 100 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 - v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lsub: {   // 0x65,       // 101 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fsub: {   // 0x66,       // 102 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dsub: {   // 0x67,       // 103 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_imul: {   // 0x68,       // 104 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 * v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lmul: {   // 0x69,       // 105 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fmul: {   // 0x6a,       // 106 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dmul: {   // 0x6b,       // 107 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_idiv: {   // 0x6c,       // 108 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 / v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_ldiv: {   // 0x6d,       // 109 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fdiv: {   // 0x6e,       // 110 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ddiv: {   // 0x6f,       // 111 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_irem: {   // 0x70,       // 112 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 % v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lrem: {   // 0x71,       // 113 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_frem: {   // 0x72,       // 114 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_drem: {   // 0x73,       // 115 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ineg: {   // 0x74,       // 116 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lneg: {   // 0x75,       // 117 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fneg: {   // 0x76,       // 118 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dneg: {   // 0x77,       // 119 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ishl: {   // 0x78,       // 120 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 << (v2 & 0x1f);
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lshl: {   // 0x79,       // 121 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ishr: {   // 0x7a,       // 122 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 >> (v2 & 0x1f);
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lshr: {   // 0x7b,       // 123 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iushr: {   // 0x7c,       // 124 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);

                uint32_t u = (uint32_t) v1;
                uint32_t r = u >> (v2 & 0x1f);
                push_int(frame, (int32_t)r);
                frame->pc++;
                break;
            }
            case OPCODE_lushr: {   // 0x7d,       // 125 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iand: {   // 0x7e,       // 126 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 & v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_land: {   // 0x7f,       // 127 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ior: {   // 0x80,       // 128 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 | v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lor: {   // 0x81,       // 129 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ixor: {   // 0x82,       // 130 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                int32_t r = v1 ^ v2;
                push_int(frame, r);
                frame->pc++;
                break;
            }
            case OPCODE_lxor: {   // 0x83,       // 131 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iinc: {   // 0x84,       // 132 
                u1 index = frame->code[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                int32_t increment = (int32_t)frame->code[frame->pc+2];
                int32_t value = (int32_t)slot->bits;
                value += increment;

                slot->bits = (uint32_t)value;
                frame->pc += 3;
                break;
            }
            // Conversions
            case OPCODE_i2l: {   // 0x85,      // 133 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_i2f: {   // 0x86,      // 134 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_i2d: {   // 0x87,      // 135 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_l2i: {   // 0x88,      // 136 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_l2f: {   // 0x89,      // 137 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_l2d: {   // 0x8a,      // 138 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_f2i: {   // 0x8b,      // 139 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_f2l: {   // 0x8c,      // 140 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_f2d: {   // 0x8d,      // 141 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_d2i: {   // 0x8e,      // 142 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_d2l: {   // 0x8f,      // 143 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_d2f: {   // 0x90,      // 144 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_i2b: {   // 0x91,      // 145 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_i2c: {   // 0x92,      // 146 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_i2s: {   // 0x93,      // 147 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Comparisons
            case OPCODE_lcmp: {   // 0x94,       // 148 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fcmpl: {   // 0x95,       // 149 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_fcmpg: {   // 0x96,       // 150 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dcmpl: {   // 0x97,       // 151 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dcmpg: {   // 0x98,       // 152 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ifeq: {   // 0x99,       // 153 
                int32_t val = pop_int(frame);
                if(val == 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_ifne: {   // 0x9a,       // 154 
                int32_t val = pop_int(frame);
                if(val != 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_iflt: {   // 0x9b,       // 155 
                int32_t val = pop_int(frame);
                if(val < 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_ifge: {   // 0x9c,       // 156 
                int32_t val = pop_int(frame);
                if(val >= 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_ifgt: {   // 0x9d,       // 157 
                int32_t val = pop_int(frame);
                if(val > 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_ifle: {   // 0x9e,       // 158 
                int32_t val = pop_int(frame);
                if(val <= 0) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmpeq: {   // 0x9f,       // 159 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 == v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmpne: {   // 0xa0,       // 160 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 != v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmplt: {   // 0xa1,       // 161 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 < v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmpge: {   // 0xa2,       // 162 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 >= v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmpgt: {   // 0xa3,       // 163 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 > v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_icmple: {   // 0xa4,       // 164 
                int32_t v2 = pop_int(frame);
                int32_t v1 = pop_int(frame);
                if(v1 <= v2) {
                    go_to_by_index(frame);
                }else{
                    frame->pc += 3;
                }
                break;
            }
            case OPCODE_if_acmpeq: {   // 0xa5,       // 165 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_if_acmpne: {   // 0xa6,       // 166 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // References
            case OPCODE_getstatic: {   // 0xb2,       // 178
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                printf("[WARN] getstatic #%d ignored.\n", index);
                // todo 所有方法调用都暂时用一个对象占位
                push_int(frame, 0xdeadbeef);
                frame->pc += 3;
                break;
            }
            case OPCODE_putstatic: {   // 0xb3,       // 179
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_getfield: {   // 0xb4,       // 180
                u1 i1 = frame->code[frame->pc+1];
                u1 i2 = frame->code[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                cp_info_t fieldref = cp_pools[index];
                check_cp_info_tag(fieldref.tag, CONSTANT_Fieldref);
                cp_info_t nametype = cp_pools[parse_to_u2(fieldref.info + 2)];
                check_cp_info_tag(nametype.tag, CONSTANT_NameAndType);
                char *field_name = get_utf8(&cp_pools[parse_to_u2(nametype.info)]);
                char *field_descriptor = get_utf8(&cp_pools[parse_to_u2(nametype.info + 2)]);

                // 找到field所在的class，获取field在实例对象中的位置和属性数量
                cp_info_t target_classref = cp_pools[parse_to_u2(fieldref.info)];
                check_cp_info_tag(target_classref.tag, CONSTANT_Class);
                class_t *target_class = load_class(get_utf8(&cp_pools[parse_to_u2(target_classref.info)]));
                field_t *target_field;
                for(u2 i = 0;i<target_class->fields_count;i++) {
                    field_t *field = &target_class->fields[i];
                    char *t_field_name = get_utf8(&target_class->cp_pools[field->name_index]);
                    char *t_field_descriptor = get_utf8(&target_class->cp_pools[field->descriptor_index]);
                    if(strcmp(field_name, t_field_name) == 0 && strcmp(field_descriptor, t_field_descriptor) == 0){
                        target_field = field;
                        break;
                    }
                }
                if(!target_field) {
                    fprintf(stderr, "cannot not found field: %s %s\n", field_name, field_descriptor);
                    abort();
                }

                object_t *ref = pop(frame)->ref;
                for(int i = target_field->slot_offset_in_class + target_field->slot_count - 1;
                    i >= target_field->slot_offset_in_class; i--) {
                    slot_t field_slot = ref->fields[i];
                    slot_t *slot = push(frame);
                    slot->bits = field_slot.bits;
                    if(field_slot.ref) {
                        memcpy(slot->ref, field_slot.ref, sizeof(object_t));
                    }
                }

                frame->pc += 3;
                break;
            }
            case OPCODE_putfield: {   // 0xb5,       // 181
                u1 i1 = frame->code[frame->pc+1];
                u1 i2 = frame->code[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                cp_info_t fieldref = cp_pools[index];
                check_cp_info_tag(fieldref.tag, CONSTANT_Fieldref);
                cp_info_t nametype = cp_pools[parse_to_u2(fieldref.info + 2)];
                check_cp_info_tag(nametype.tag, CONSTANT_NameAndType);
                char *field_name = get_utf8(&cp_pools[parse_to_u2(nametype.info)]);
                char *field_descriptor = get_utf8(&cp_pools[parse_to_u2(nametype.info + 2)]);

                // 找到field所在的class，获取field在实例对象中的位置和属性数量
                cp_info_t target_classref = cp_pools[parse_to_u2(fieldref.info)];
                check_cp_info_tag(target_classref.tag, CONSTANT_Class);
                class_t *target_class = load_class(get_utf8(&cp_pools[parse_to_u2(target_classref.info)]));
                field_t *target_field;
                for(u2 i = 0;i<target_class->fields_count;i++) {
                    field_t *field = &target_class->fields[i];
                    char *t_field_name = get_utf8(&target_class->cp_pools[field->name_index]);
                    char *t_field_descriptor = get_utf8(&target_class->cp_pools[field->descriptor_index]);
                    if(strcmp(field_name, t_field_name) == 0 && strcmp(field_descriptor, t_field_descriptor) == 0){
                        target_field = field;
                        break;
                    }
                }
                if(!target_field) {
                    fprintf(stderr, "cannot not found field: %s %s\n", field_name, field_descriptor);
                    abort();
                }

                // 从当前堆栈中，pop出指定数量的slot
                slot_t argc[target_field->slot_count];
                index = 0;
                for(u2 i =0;i<target_field->slot_count;i++) {
                    slot_t *slot = pop(frame);
                    argc[i].bits = slot->bits;
                    if(slot->ref) {
                        memcpy(argc[i].ref, slot->ref, sizeof(object_t));
                    }
                }

                // 把slot的信息写到实例对象中
                slot_t *objref = pop(frame);
                object_t *ref = objref->ref;
                for(u2 i=0;i<target_field->slot_count;i++) {
                    slot_t field_slot = ref->fields[target_field->slot_offset_in_class + i];
                    slot_t arg = argc[i];
                    field_slot.bits = arg.bits;
                    if(arg.ref) {
                        memcpy(field_slot.ref, arg.ref, sizeof(object_t));
                    }
                }

                frame->pc += 3;
                break;
            }
            case OPCODE_invokevirtual: {   // 0xb6,       // 182
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                cp_info_t info = cp_pools[index];
                check_cp_info_tag(info.tag, CONSTANT_Methodref);
                info = cp_pools[parse_to_u2(info.info + 2)];
                check_cp_info_tag(info.tag, CONSTANT_NameAndType);
                char *name = get_utf8(&cp_pools[parse_to_u2(info.info)]);
                u2 arg_slot_count = slot_count_from_desciptor(get_utf8(&cp_pools[parse_to_u2(info.info + 2)]));
                for(int i=0;i<arg_slot_count;i++) {
                    int32_t arg = pop_int(frame);
                    if(strcmp(name, "println") == 0) {
                        printf("%d\n", arg);
                    }
                }
                int32_t obj = pop_int(frame);
                // todo 暂时用不到
                (void) obj;
                frame->pc += 3;
                break;
            }
            case OPCODE_invokespecial: {   // 0xb7,       // 183
                u2 index1 = frame->code[frame->pc+1];
                u2 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                cp_info_t methodref = cp_pools[index];
                check_cp_info_tag(methodref.tag, CONSTANT_Methodref);

                // class index
                index = parse_to_u2(methodref.info);
                cp_info_t classref = cp_pools[index];
                check_cp_info_tag(classref.tag, CONSTANT_Class);
                index = parse_to_u2(classref.info);
                char *c_name = get_utf8(&cp_pools[index]);
                printf("class name: %s\n", c_name);

                // name and type index
                index = parse_to_u2(methodref.info + 2);
                cp_info_t nametype = cp_pools[index];
                check_cp_info_tag(nametype.tag, CONSTANT_NameAndType);
                u2 name_index = (nametype.info[0] << 8) | nametype.info[1];
                u2 descriptor_index = (nametype.info[2] << 8) | nametype.info[3];
                method_t *call_method;
                for(int i=0;i<class->methods_count;i++) {
                    method_t *method = &class->methods[i];
                    if(method->name_index == name_index && method->descriptor_index == descriptor_index) {
                        call_method = method;
                        break;
                    }
                }
                char *m_name = get_utf8(&cp_pools[name_index]);
                if(call_method == NULL) {
                    fprintf(stderr, "Error: method not found: %s\n", m_name);
                    abort();
                }
                // 1. 获取方法名和类名
                // 假设你有办法从 method 拿到它所属的 class 结构
                // char *c_name = get_class_name(method->owner_class); 
                printf("invokespecial: %s\n", m_name);

                frame_t *sub_frame = create_frame(call_method, frame);
                slot_t *this_slot = get_local(sub_frame, 0);
    
                if (this_slot->ref == NULL) {
                    fprintf(stderr, "Error: 'this' is NULL in invokespecial\n");
                    abort();
                }

                // 2. 补丁：如果是 Object 的初始化，直接返回，不进入 interpret
                // 这里的判断逻辑需要根据你的 class 结构完善
                if (strcmp(m_name, "<init>") == 0) {
                    // 如果类名是 java/lang/Object，则不递归
                    if(strcmp(c_name, "java/lang/Object") == 0) {
                        return;
                    }
                }
                // abort();
                interpret(sub_frame, this_slot->ref->class);
                frame_free(sub_frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_invokestatic: {   // 0xb8,       // 184
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                cp_info_t info = cp_pools[index];
                method_t *method = find_method(class, &info);
                frame_t *sub_frame = create_frame(method, frame);
                interpret(sub_frame, class);
                frame_free(sub_frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_invokeinterface: {   // 0xb9,       // 185
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_invokedynamic: {   // 0xba,       // 186
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_new: {   // 0xbb,       // 187
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                cp_info_t info = cp_pools[index];
                check_cp_info_tag(info.tag, CONSTANT_Class);
                char *class_name = get_utf8(&cp_pools[parse_to_u2(info.info)]);
                class_t *local_class = load_class(class_name);
                object_t *ref = calloc(1, sizeof(object_t));
                ref->class = local_class;
                ref->fields = calloc(local_class->total_field_slots, sizeof(slot_t));
                push(frame)->ref = ref;
                frame->pc += 3;
                break;
            }
            case OPCODE_newarray: {   // 0xbc,       // 188
                int32_t a_count = pop_int(frame);
                if(a_count < 0) {
                    // todo throw exception
                    abort();
                }
                slot_t *slot = push(frame);
                if(!slot->ref) {
                    slot->ref = calloc(1, sizeof(object_t));
                }
                u1 atype = frame->code[frame->pc + 1];
                slot->ref->atype = atype;
                switch(atype) {
                    case ATYPE_BOOLEAN:
                    case ATYPE_BYTE:
                    case ATYPE_CHAR:
                    case ATYPE_SHORT:
                    case ATYPE_INT:
                    case ATYPE_FLOAT:
                        break;
                    case ATYPE_DOUBLE:
                    case ATYPE_LONG:
                        a_count *= 2;
                        break;
                    default:
                        // todo throw exception
                        abort();
                }
                slot->ref->acount = a_count;
                slot->ref->fields = calloc(a_count, sizeof(slot_t));
                frame->pc += 2;
                break;
            }
            case OPCODE_anewarray: {   // 0xbd,       // 189
                int32_t count = pop_int(frame);
                u1 high = frame->code[frame->pc + 1];
                u1 low = frame->code[frame->pc + 2];
                u2 index = (high << 8) | low;
                cp_info_t cp_info = cp_pools[index];
                check_cp_info_tag(cp_info.tag, CONSTANT_Class);
                class_t *local_class = load_class(get_utf8(&cp_pools[parse_to_u2(cp_info.info)]));
                slot_t *slot = push(frame);
                slot->ref->acount = count;
                slot->ref->class = local_class;
                slot->ref->fields = calloc(1, count * sizeof(slot_t));
                slot->ref->atype = ATYPE_REF;
                frame->pc += 3;
                break;
            }
            case OPCODE_arraylength: {   // 0xbe,       // 190
                object_t *ref = pop(frame)->ref;
                push_int(frame, ref->acount);
                frame->pc++;
                break;
            }
            case OPCODE_athrow: {   // 0xbf,       // 191
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_checkcast: {   // 0xc0,       // 192
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_instanceof: {   // 0xc1,       // 193
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_monitorenter: {   // 0xc2,       // 194
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_monitorexit: {   // 0xc3,       // 195
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Control
            case OPCODE_goto: {   // 0xa7,       // 167 
                u1 index1 = frame->code[frame->pc+1];
                u1 index2 = frame->code[frame->pc+2];
                int16_t index = (int16_t)((index1 << 8) | index2);
                frame->pc += index;
                break;
            }
            case OPCODE_jsr: {   // 0xa8,       // 168 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ret: {   // 0xa9,       // 169 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_tableswitch: {   // 0xaa,       // 170 
                u2 offset = 1;
                while((frame->pc + offset) % 4 != 0) {
                    offset++;
                }
                int32_t def_val = (int32_t)parse_to_u4(frame->code + frame->pc + offset);
                offset += 4;
                int32_t low = (int32_t)parse_to_u4(frame->code + frame->pc + offset);
                offset += 4;
                int32_t high = (int32_t)parse_to_u4(frame->code + frame->pc + offset);
                u2 length = high - low + 1;
                int32_t index = pop_int(frame);
                if(index < low || index > high) {
                    frame->pc += def_val;
                }else{
                    // jump table
                    offset += 4;
                    offset += (index - low) * 4;
                    int32_t jump_offset = (int32_t)parse_to_u4(frame->code + frame->pc + offset);
                    frame->pc += jump_offset;
                }
                break;
            }
            case OPCODE_lookupswitch: {   // 0xab,       // 171 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ireturn: {   // 0xac,       // 172 
                frame_t *invoke = frame->invoker;
                push_int(invoke, pop_int(frame));
                return;
            }
            case OPCODE_lreturn: {   // 0xad,       // 173 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_freturn: {   // 0xae,       // 174 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_dreturn: {   // 0xaf,       // 175 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_areturn: {   // 0xb0,       // 176 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_return: {   // 0xb1,       // 177 
                return;
            }
            // Extended
            case OPCODE_wide: {   // 0xc4,      // 196 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_multianewarray: {   // 0xc5,      // 197 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ifnull: {   // 0xc6,      // 198 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ifnonnull: {   // 0xc7,      // 199 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_goto_w: {   // 0xc8,      // 200 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_jsr_w: {   // 0xc9,      // 201 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            // Reserved
            case OPCODE_breakpoint: {   // 0xca,     // 202 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_impdep1: {   // 0xfe,     // 254 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_impdep2: {   // 0xff,     // 255 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            default: 
                fprintf(stderr, "unknown opcode: %d\n", opcode);
                abort();
        }
    }
}

void dump_frame(frame_t *frame) {
    printf("[DUMP] frame: \n");
    printf("[DUMP] local vars: %d\n", frame->local_var_size);
    printf("[DUMP] pc: %d / %d\n", frame->pc, frame->code_length);
    printf("[DUMP] sp: %d / %d\n", frame->sp, frame->operand_stack_size);
    printf("[DUMP] opcode: %d\n", frame->code[frame->pc]);
}

void print_operand_stack(frame_t *frame) {
    printf("[DUMP] operand stack: %p\n", frame->operand_stack);
    for(int i=0;i<frame->sp;i++) {
        printf("[DUMP] operand stack: %d, %p\n", i, &frame->operand_stack[i]);
    }
}


void print_stacktrace(void) {
    void *buffer[64];
    int nptrs = backtrace(buffer, 64);
    char **symbols = backtrace_symbols(buffer, nptrs);

    if (symbols == NULL) {
        perror("backtrace_symbols");
        return;
    }

    for (int i = 0; i < nptrs; i++) {
        printf("%s\n", symbols[i]);
    }

    free(symbols);
}

void dump_code_hex(u1 *codes) {
    printf("[DUMP] code: ");
    for(int i=0;i<codes[0];i++) {
        printf("%02x ", codes[i]);
    }
    printf("\n");
}