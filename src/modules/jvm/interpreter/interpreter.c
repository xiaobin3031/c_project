#include "interpreter.h"
#include "opcode.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../classfile/constant_pool.h"
#include "../classfile/attr.h"
#include "../runtime/frame.h"
#include "../vm/vm.h"
#include "../utils/slots.h"
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

void interpret(frame_t *frame, class_t *class) {
    if(frame->code_length == 0) return;

    u1 opcode;
    cp_info_t *cp_pools = class->cp_pools;

    while(frame->pc < frame->code_length) {
        opcode = frame->code[frame->pc];
        printf("opcode: %d\n", opcode);
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
            case OPCODE_istore_0: {
                slot_t *local_slot = get_local(frame, 0);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_1: {
                slot_t *local_slot = get_local(frame, 1);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_2: {
                slot_t *local_slot = get_local(frame, 2);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_istore_3: {
                slot_t *local_slot = get_local(frame, 3);
                local_slot->bits = pop(frame)->bits;
                frame->pc++;
                break;
            }
            case OPCODE_astore: {
                u1 index = frame->code[frame->pc + 1];
                get_local(frame, index)->ref = pop(frame)->ref;
                frame->pc += 2;
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
            case OPCODE_aload: {
                u1 index = frame->code[frame->pc + 1];
                push(frame)->ref = get_local(frame, index)->ref;
                frame->pc += 2;
                break;
            }
            case OPCODE_aload_0: {
                push(frame)->ref = get_local(frame, 0)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_1: {
                push(frame)->ref = get_local(frame, 1)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_2: {
                push(frame)->ref = get_local(frame, 2)->ref;
                frame->pc++;
                break;
            }
            case OPCODE_aload_3: {
                push(frame)->ref = get_local(frame, 3)->ref;
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
                int32_t bb = (int32_t)frame->code[frame->pc+1];
                push(frame)->bits = (uint32_t)bb;
                frame->pc += 2;
                break;
            }
            case OPCODE_sipush: {
                u1 v1 = frame->code[frame->pc+1];
                u1 v2 = frame->code[frame->pc+2];
                u2 r = (v1 << 8) | v2;
                push_int(frame, (int32_t)r);
                frame->pc += 3;
                break;
            }
            case OPCODE_pop: {
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_dup: {
                slot_t *slot = pop(frame);
                push(frame);
                slot_t *dup_slot = push(frame);
                dup_slot->bits = slot->bits;
                if(slot->ref) {
                    dup_slot->ref = malloc(sizeof(slot->ref));
                    memcpy(dup_slot->ref, slot->ref, sizeof(slot->ref));
                }
                frame->pc++;
                break;
            }
            case OPCODE_iinc: { 
                u1 index = frame->code[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                int32_t increment = (int32_t)frame->code[frame->pc+2];
                int32_t value = (int32_t)slot->bits;
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
                    u1 bb2 = frame->code[frame->pc+2];
                    int16_t index = (int16_t)((bb1 << 8) | bb2);
                    frame->pc += index;
                }else{
                    frame->pc += 3;
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
                    frame->pc += 3;
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
                    frame->pc += 3;
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
                    frame->pc += 3;
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
                    frame->pc += 3;
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
                    frame->pc += 3;
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
                // todo 所有方法调用都暂时用一个对象占位
                push_int(frame, 0xdeadbeef);
                frame->pc += 3;
                break;
            }
            case OPCODE_invokevirtual: {
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
            case OPCODE_invokestatic: {
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
            case OPCODE_invokespecial: {
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
            case OPCODE_new: {
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
            case OPCODE_ireturn: {
                frame_t *invoke = frame->invoker;
                push_int(invoke, pop_int(frame));
                return;
            }
            case OPCODE_return: {
                return;
            }
            case OPCODE_nop: {
                frame->pc++;
                break;
            }
            case OPCODE_getfield: {
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
                        memcpy(slot->ref, field_slot.ref, sizeof(field_slot.ref));
                    }
                }

                frame->pc += 3;
                break;
            }
            case OPCODE_putfield: {
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
                        memcpy(argc[i].ref, slot->ref, sizeof(slot->ref));
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
                        memcpy(field_slot.ref, arg.ref, sizeof(arg.ref));
                    }
                }

                frame->pc += 3;
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