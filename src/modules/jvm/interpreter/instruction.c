#include "interpreter.h"
#include "opcode.h"
#include "../runtime/frame.h"
#include "../runtime/operand_stack.h"
#include "../runtime/local_vars.h"
#include "../runtime/class.h"
#include "../vm/classload.h"
#include "../utils/slots.h"
#include "../runtime/native.h"
#include "../runtime/jmemory.h"
#include "../../../core/list/arraylist.h"
#include "../junit_create/junit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>
#include <stdarg.h>

void throw_error(jvm_thread_t *thread, enum run_error_e type, const char *message, ...) {
    va_list args;

    thread->error = error_new(type, NULL);
    if(message != NULL) {
        va_start(args, message);
        char *formatted_message = NULL;
        vasprintf(&formatted_message, message, args);
        va_end(args);

        thread->error->message = formatted_message;
        printf("thread run error: %s\n", formatted_message);
    }
}

class_t *resolve_class(jvm_thread_t *thread, rt_cp_entry_t *entry) {
    if(entry->resolved == 1) return entry->klass;

    // printf("resolve class: %s\n", entry->sym.class_name);
    // 直接设置class
    class_t *target_class = load_class(entry->sym.class_name, thread);
    if(target_class->state == CLASS_ERRONEOUS) {
        return NULL;
    }
    entry->klass = target_class;
    entry->resolved = 1;
    return target_class;
}

static field_t *resolve_field(jvm_thread_t *thread, rt_cp_entry_t *entry) {
    if(entry->resolved == 1) return entry->field;

    class_t *target_class = resolve_class(thread, entry);
    while(target_class != NULL) {
        ensure_class_initialized(target_class, thread);
        if(target_class->state == CLASS_ERRONEOUS) {
            return NULL;
        }
        for(u2 i = 0; i < target_class->fields_count; i++) {
            field_t *local_field = &target_class->fields[i];
            if(strcmp(entry->sym.name, local_field->name) == 0 && strcmp(entry->sym.descriptor, local_field->descriptor) == 0) {
                entry->field = local_field;
                entry->resolved = 1;
                return local_field;
            }
        }

        target_class = target_class->super;
    }

    // printf("field not found: %s %s\n", entry->sym.class_name, entry->sym.name);
    // abort();
    return NULL;
}

static method_t *resolve_method(jvm_thread_t *thread, class_t *class, rt_cp_entry_t *entry) {
    if(entry->resolved == 1) return entry->method;

    class_t *target_class = resolve_class(thread, entry);
    method_t *method = calloc(1, sizeof(method_t));
    method->access_flags = METHOD_ACC_PUBLIC;
    method->name = strdup(entry->sym.name);
    method->klass = target_class;
    method->descriptor = strdup(entry->sym.descriptor);
    method->arg_slot_count = slot_count_from_desciptor(method->descriptor);
    char *end = strrchr(method->descriptor, ')');
    method->return_slot_count = slot_count_from_desciptor(end + 1);

    entry->method = method;
    entry->resolved = 1;

    return method;
}

static field_t *find_static_field(jvm_thread_t *thread, rt_cp_entry_t *entry) {
    field_t *field = resolve_field(thread, entry);
    if(field == NULL) return NULL;

    if(field->access_flags & FIELD_ACC_STATIC) {
        return field;
    }

    throw_error(thread, RUNTIME_ERROR_IncompatibleClassChangeError, "field is not static");
    return NULL;
}

static int run_method(jvm_thread_t *thread, u2 methodref_index, frame_t *cur_frame, int is_static) {
    class_t *class = thread->current_frame->method->klass;
    rt_cp_entry_t *entry = &class->entries[methodref_index];
    method_t *call_method = resolve_method(thread, class, entry);

    // 只执行private，其他方法根据是否需要mock来判断
    if(call_method->access_flags & METHOD_ACC_PRIVATE) {
        frame_t *run_frame = frame_new(call_method, cur_frame);
        push_frame(thread, run_frame);
        return 0;
    }else{
        printf("invoke method: %s.%s%s\n", call_method->klass->class_name, call_method->name, call_method->descriptor);
        // 因为不是static的
        if(!is_static) pop(cur_frame);
        if(call_method->arg_slot_count > 0) {
            for(int i = 0; i < call_method->arg_slot_count; i++) {
                pop(cur_frame);
            }
        }
        if(call_method->return_slot_count > 0) {
            for(int i = 0; i < call_method->return_slot_count; i++) {
                push(cur_frame);
            }
        }
        return 0;
    }
}

static u2 get_new_pc(frame_t *frame) {
    rt_code_t *rt_code = frame->method->code;
    u1 bb1 = rt_code->codes[frame->pc+1];
    u1 bb2 = rt_code->codes[frame->pc+2];
    int16_t index = (int16_t)((bb1 << 8) | bb2);
    return frame->pc + index;
}

static int is_class_assignable(class_t *from, class_t *to) {
    if(to->access_flags && CLASS_ACC_INTERFACE) {
        // from实现了to接口
        for(int i=0;i<from->interface_count;i++) {
            class_t *interface = from->interface_class[i];
            if(interface == to) return 1;
        }

    }else {
        if(from == to) {
            return 1;
        }
        for(class_t *super = from->super; super; super = super->super) {
            if(super == to) return 1;
        }
    }

    return 0;
}

static int is_assignable(class_t *from, class_t *to) {
    // to must be class, array, or interface
    if(from->is_array) {
        if(to->is_array) {
            return is_class_assignable(from, to);
        }else {
            return (strcmp(to->class_name, "java/lang/Object") == 0
                || strcmp(to->class_name, "java/lang/Cloneable") == 0
                || strcmp(to->class_name, "java/io/Serializable") == 0) ? 1 : 0;
        }
    }
    return is_class_assignable(from, to);
}

static void push_stack(frame_t *frame, u2 index) {
    slot_t *local = get_local(frame, index);
    slot_t *stack = push(frame);
    stack->bits = local->bits;
    stack->ref = local->ref;
    stack->test_field = local->test_field;
    stack->vt = local->vt;
}

static void pop_stack(frame_t *frame, u2 index) {
    slot_t *local = get_local(frame, index);
    slot_t *stack = pop(frame);
    local->bits = stack->bits;
    local->ref = stack->ref;
    local->test_field = stack->test_field;
    stack->vt = local->vt;
}

/**
 * 记录分支信息
 * @param ok_pc taken == 0
 * @param fail_pc taken == 1
 */
static if_t *record_branch(slot_t *slot, test_method_t *test_method, const char *if_name, u2 pc, u2 ok_pc, u2 fail_pc) {
    // 从之前的记录中找到if
    arraylist *allifs = test_method->all_ifs;
    if_t *ift = NULL;
    for(size_t i=0;i<allifs->size;i++) {
        if_t *tmp = arraylist_get(allifs, i);
        if(tmp->pc == pc) {
            ift = tmp;
            break;
        }
    }

    if(ift == NULL) {
        ift = calloc(1, sizeof(if_t));
        ift->if_name = strdup(if_name);
        ift->pc = pc;
        // 保存到这个方法的所有分支里，后续判断是否结束，以及判断是否已经遍历过了
        arraylist_add(allifs, ift);
    }
    ift->vt = slot->vt;

    // if(ift->taken == 0) {
    //     int match = 0;
    //     for(size_t i=0;i<allifs->size;i++) {
    //         if_t *tmp = arraylist_get(allifs, i);
    //         if(tmp->pc != pc && tmp->get_pcs[0] == ok_pc) {
    //             match = 1;
    //             break;
    //         }
    //     }
    //     if(match == 1) {
    //         // 0已经有人走过了，该走1了
    //         ift->taken = 1;
    //     }
    // }
    // if(ift->taken == 1) {
    //     int match = 0;
    //     for(size_t i=0;i<allifs->size;i++) {
    //         if_t *tmp = arraylist_get(allifs, i);
    //         if(tmp->pc != pc && tmp->get_pcs[1] == fail_pc) {
    //             match = 1;
    //             break;
    //         }
    //     }
    //     if(match == 1) {
    //         // 1已经有人走过了，该结束了
    //         ift->taken = 2;
    //     }
    // }
    body_branch_t *body_branch = calloc(1, sizeof(body_branch_t));
    body_branch->kind = BODY_BRANCH_IF;
    body_branch->if_branch = ift;
    arraylist_add(test_method->branchs, body_branch);
    return ift;
}


/**
 * 记录比较节点
 */
static if_t *record_compare_branch(slot_t *slot, slot_t *slot2, const char *if_name, frame_t *frame, u2 opcode, u2 ok_pc, u2 fail_pc) {
    // 从之前的记录中找到if
    arraylist *allifs = frame->test_method->all_ifs;
    if_t *ift = NULL;
    for(size_t i=0;i<allifs->size;i++) {
        if_t *tmp = arraylist_get(allifs, i);
        if(tmp->pc == frame->pc) {
            ift = tmp;
            break;
        }
    }

    if(ift == NULL) {
        ift = calloc(1, sizeof(if_t));
        ift->if_name = strdup(if_name);
        ift->pc = frame->pc;
        // 保存到这个方法的所有分支里，后续判断是否结束，以及判断是否已经遍历过了
        arraylist_add(allifs, ift);
    }

    value_trace_t *vt = vt_compare_new(opcode, slot->vt, slot2->vt);
    ift->vt = vt;

    // if(ift->taken == 0) {
    //     int match = 0;
    //     for(size_t i=0;i<allifs->size;i++) {
    //         if_t *tmp = arraylist_get(allifs, i);
    //         if(tmp->pc != frame->pc && tmp->get_pcs[0] == ok_pc) {
    //             match = 1;
    //             break;
    //         }
    //     }
    //     if(match == 1) {
    //         // 0已经有人走过了，该走1了
    //         ift->taken = 1;
    //     }
    // }
    // if(ift->taken == 1) {
    //     int match = 0;
    //     for(size_t i=0;i<allifs->size;i++) {
    //         if_t *tmp = arraylist_get(allifs, i);
    //         if(tmp->pc != frame->pc && tmp->get_pcs[1] == fail_pc) {
    //             match = 1;
    //             break;
    //         }
    //     }
    //     if(match == 1) {
    //         // 1已经有人走过了，该结束了
    //         ift->taken = 2;
    //     }
    // }

    body_branch_t *body_branch = calloc(1, sizeof(body_branch_t));
    body_branch->kind = BODY_BRANCH_IF;
    body_branch->if_branch = ift;
    arraylist_add(frame->test_method->branchs, body_branch);
    return ift;
}

static int opcode_is_store(u1 opcode) {
    return opcode == OPCODE_istore || opcode == OPCODE_lstore || opcode == OPCODE_fstore || opcode == OPCODE_dstore || opcode == OPCODE_astore
        || opcode == OPCODE_bastore || opcode == OPCODE_castore || opcode == OPCODE_sastore;
}

static void fill_expr_init(var_expr_t *expr_init, const char *type) {
    if(strcmp(type, "Ljava/lang/String;") == 0) {
        expr_init->type = "String";
        literal_expr_t *literal_expr = literal_expr_new(LIT_STRING);
        literal_expr->s = "1";
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/util/List;") == 0) {
        expr_init->type = "List";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("ArrayList", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/util/Map;") == 0) {
        expr_init->type = "Map";
        expr_init->params = arraylist_new(2);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("HashMap", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/util/Set;") == 0) {
        expr_init->type = "Set";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("HashSet", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/lang/Integer") == 0) {
        expr_init->type = "int";
        literal_expr_t *literal_expr = literal_expr_new(LIT_INT);
        literal_expr->i = 1;
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/lang/Long") == 0) {
        expr_init->type = "long";
        literal_expr_t *literal_expr = literal_expr_new(LIT_LONG);
        literal_expr->l = 1L;
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strcmp(type, "Ljava/math/BigDecimal;") == 0) {
        expr_init->type = "BigDecimal";
        method_call_expr_t *mc = method_call_expr_new(NULL, "BigDecimal.valueOf");
        arraylist_add(mc->args, "1");
        expr_t *expr = expr_new(EXPR_METHOD_CALL);
        expr->method_call = mc;
        expr_init->init = expr;
    }else if(strcmp(type, "Lorg/redisson/api/RBucket;") == 0) {
        expr_init->type = "RBucket";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "Object");
        expr_new_t *exprnew = expr_new_new("MyRBucket", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
    }
    else {
        char buffer[200];
        expr_init->type = descriptor_to_simple_type(type);
        sprintf(buffer, "%s.class", expr_init->type);
        method_call_expr_t *mc = method_call_expr_new(NULL, "Util.newAndInit");
        arraylist_add(mc->args, strdup(buffer));
        expr_t *expr = expr_new(EXPR_METHOD_CALL);
        expr->method_call = mc;
        expr_init->init = expr;
    }
}

void exec_instruction(jvm_thread_t *thread) {
    frame_t *frame = thread->current_frame;
    u4 code_length = frame->method->code->code_length;
    if(code_length == 0) return;

    u1 *codes = frame->method->code->codes;
    u1 opcode;
    class_t *class = frame->method->klass;
    rt_cp_entry_t *entries = class->entries;

    while(frame->pc < code_length) {
        opcode = codes[frame->pc];
        printf("%d: opcode: %d %s\n", frame->pc, opcode, opcode_to_string(opcode));
        switch(opcode) {
            // Constants
            case OPCODE_nop: {   // 0x00,  // 00 
                frame->pc++;
                break;
            }
            case OPCODE_aconst_null: {   // 0x01,  // 01 
                slot_t *slot = push(frame);
                slot->bits = 0x0;
                slot->ref = NULL;
                frame->pc++;
                break;
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
                int32_t bb = (int32_t)codes[frame->pc+1];
                printf("bipush: %d\n", bb);
                slot_t *slot = push(frame);
                slot->bits = (uint32_t)bb;
                slot->vt = vt_const_new(codes[frame->pc+1]);
                slot->vt->constant.sub_kind = VT_SUB_CONST_INT;
                frame->pc += 2;
                break;
            }
            case OPCODE_sipush: {   // 0x11,  // 17 
                u1 v1 = codes[frame->pc+1];
                u1 v2 = codes[frame->pc+2];
                u2 r = (v1 << 8) | v2;
                push_int(frame, (int32_t)r);
                frame->pc += 3;
                break;
            }
            case OPCODE_ldc: {   // 0x12,  // 18 
                u1 index = codes[frame->pc+1];
                rt_cp_entry_t *entry = &entries[index];
                // printf("ldc cp_info.tag: %d\n", cp_info.tag);
                if(entry->tag == RT_CONSTANT_Integer) {
                    push_int(frame, entry->number.int_value);
                }else if(entry->tag == RT_CONSTANT_Float) {
                    push_float(frame, entry->number.float_value);
                }else if(entry->tag == RT_CONSTANT_String) {
                    slot_t *slot = push(frame);
                    object_t *ref = heap_alloc_object(load_class("java/lang/String", thread));
                    ref->native_string = entry->strings;
                    slot->ref = ref;
                }else if(entry->tag == RT_CONSTANT_Class) {
                    slot_t *slot = push(frame);
                    class_t *klass = resolve_class(thread, entry);
                    if(slot->ref == NULL) {
                        slot->ref = heap_alloc_object(klass);
                    }else{
                        slot->ref->klass = klass;
                    }
                }else if(entry->tag == RT_CONSTANT_MethodType) {
                    fprintf(stderr, "ldc method type not implemented\n");
                    abort();
                }else if(entry->tag == RT_CONSTANT_MethodHandle) {
                    fprintf(stderr, "ldc method handle not implemented\n");
                    abort();
                }else if(entry->tag == RT_CONSTANT_InvokeDynamic) {
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
                u2 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                rt_cp_entry_t *entry = entries + index;
                if(entry->tag == RT_CONSTANT_Long) {
                    push_long(frame, entry->number.long_value);
                }else if(entry->tag == RT_CONSTANT_Double) {
                    push_double(frame, entry->number.double_value);
                }else {
                    throw_error(thread, RUNTIME_ERROR_RuntimeException, "ldc2_w tag error, not long or double");
                }
                frame->pc += 3;
                break;
            }
            // Loads
            case OPCODE_iload: {   // 0x15,     // 21 
                u1 index = codes[frame->pc+1];
                push_stack(frame, index);
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
                u1 index = codes[frame->pc + 1];
                push_stack(frame, index);
                frame->pc += 2;
                break;
            }
            case OPCODE_iload_0: {   // 0x1a,     // 26 
                push_stack(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_iload_1: {   // 0x1b,     // 27 
                push_stack(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_iload_2: {   // 0x1c,     // 28 
                push_stack(frame, 2);
                frame->pc++;
                break;
            }
            case OPCODE_iload_3: {   // 0x1d,     // 29 
                push_stack(frame, 3);
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
                push_stack(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_aload_1: {   // 0x2b,     // 43 
                push_stack(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_aload_2: {   // 0x2c,     // 44 
                push_stack(frame, 2);
                frame->pc++;
                break;
            }
            case OPCODE_aload_3: {   // 0x2d,     // 45 
                push_stack(frame, 3);
                frame->pc++;
                break;
            }
            case OPCODE_iaload: {   // 0x2e,     // 46 
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                if(array == NULL) {
                    throw_error(thread, RUNTIME_ERROR_NullPointerException, NULL);
                    return;
                }
                if(index < 0 || index >= array->array.length) {
                    throw_error(thread, RUNTIME_ERROR_ArrayIndexOutOfBoundsException, NULL);
                    return;
                }
                slot_t *slot = &array->array.elements[index];
                push(frame)->bits = slot->bits;
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
                slot->ref = array->array.elements[index].ref;
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
                u1 index = codes[frame->pc+1];
                pop_stack(frame, index);
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
                u1 index = codes[frame->pc + 1];
                pop_stack(frame, index);
                frame->pc += 2;
                break;
            }
            case OPCODE_istore_0: {   // 0x3b,       // 59 
                pop_stack(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_istore_1: {   // 0x3c,       // 60 
                pop_stack(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_istore_2: {   // 0x3d,       // 61 
                pop_stack(frame, 2);
                frame->pc++;
                break;
            }
            case OPCODE_istore_3: {   // 0x3e,       // 62 
                pop_stack(frame, 3);
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
                pop_stack(frame, 0);
                frame->pc++;
                break;
            }
            case OPCODE_astore_1: {   // 0x4c,       // 76 
                pop_stack(frame, 1);
                frame->pc++;
                break;
            }
            case OPCODE_astore_2: {   // 0x4d,       // 77 
                pop_stack(frame, 2);
                frame->pc++;
                break;
            }
            case OPCODE_astore_3: {   // 0x4e,       // 78 
                pop_stack(frame, 3);
                frame->pc++;
                break;
            }
            case OPCODE_iastore: {   // 0x4f,       // 79 
                int32_t value = pop_int(frame);
                int32_t index = pop_int(frame);
                object_t *array = pop(frame)->ref;
                if(index >= array->array.length) {
                    // TODO: throw exception
                    abort();
                }
                slot_t *slot = &array->array.elements[index];
                slot->bits = (uint32_t)value;
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
                (&array->array.elements[index])->ref = value;
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
                u1 index = codes[frame->pc+1];
                slot_t *slot = get_local(frame, index);
                int32_t increment = (int32_t)codes[frame->pc+2];
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
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "ifeq", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    ift->vt->value = vt_const_new(0);
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    ift->vt->value = vt_const_new(1);
                    frame->pc += 3;
                }else{
                    // 已经结束了，返回
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_ifne: {   // 0x9a,       // 154 
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "ifne", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    ift->vt->value = vt_const_new(1);
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    ift->vt->value = vt_const_new(0);
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_iflt: {   // 0x9b,       // 155 
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "iflt", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_ifge: {   // 0x9c,       // 156 
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "ifge", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_ifgt: {   // 0x9d,       // 157 
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "ifgt", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_ifle: {   // 0x9e,       // 158 
                slot_t *slot = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_branch(slot, frame->test_method, "ifle", frame->pc, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmpeq: {   // 0x9f,       // 159 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmpeq", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    ift->vt->value = vt_const_new(0);
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    ift->vt->value = vt_const_new(1);
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmpne: {   // 0xa0,       // 160 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmpne", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmplt: {   // 0xa1,       // 161 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmplt", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmpge: {   // 0xa2,       // 162 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmpge", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmpgt: {   // 0xa3,       // 163 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmpgt", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_icmple: {   // 0xa4,       // 164 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "icmple", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_acmpeq: {   // 0xa5,       // 165 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "acmpeq", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            case OPCODE_if_acmpne: {   // 0xa6,       // 166 
                slot_t *slot2 = pop(frame);
                slot_t *slot1 = pop(frame);
                u2 ok_pc = get_new_pc(frame);
                if_t *ift = record_compare_branch(slot1, slot2, "acmpne", frame, opcode, ok_pc, frame->pc + 3);
                if(ift->taken == 0) {
                    frame->pc = ok_pc;
                }else if(ift->taken == 1){
                    frame->pc += 3;
                }else{
                    frame->test_method->short_circuit = 1;
                    return;
                }
                ift->get_pcs[ift->taken] = frame->pc;
                break;
            }
            // References
            case OPCODE_getstatic: {   // 0xb2,       // 178
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                field_t *field = find_static_field(thread, entries + index);
                if(field == NULL) return;

                slot_t *slots = field->slots;
                for(int i=field->slot_count-1; i>=0; i--) {
                    slot_t *stack_slot = push(frame);
                    slot_t *field_slot = field->slots + i;
                    stack_slot->bits = field_slot->bits;
                    stack_slot->ref = field_slot->ref;
                }

                frame->pc += 3;
                break;
            }
            case OPCODE_putstatic: {   // 0xb3,       // 179
                u1 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                field_t *field = find_static_field(thread, entries + index);
                if(field == NULL) return;

                for(int i = 0; i < field->slot_count; i++) {
                    slot_t *stack_slot = pop(frame);
                    slot_t *field_slot = field->slots + i;
                    field_slot->bits = stack_slot->bits;
                    field_slot->ref = stack_slot->ref;
                }
                frame->pc += 3;
                break;
            }
            case OPCODE_getfield: {   // 0xb4,       // 180
                u1 i1 = codes[frame->pc+1];
                u1 i2 = codes[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                frame->pc += 3;
                field_t *target_field = resolve_field(thread, entries + index);
                // 判断是否是mock过的
                arraylist *test_fields = frame->test_class->fields;
                test_field_t *match_field = NULL;
                for(size_t i=0;i<test_fields->size;i++) {
                    test_field_t *field = (test_field_t *)arraylist_get(test_fields, i);
                    if(strcmp(field->name, target_field->name) == 0 && strcmp(field->descriptor, target_field->descriptor) == 0) {
                        match_field = field;
                        break;
                    }
                }

                pop(frame);
                // 模拟的情况下，这里的ref可能是个null
                // object_t *ref = pop(frame)->ref;
                // if(ref == NULL) {
                //     throw_error(thread, RUNTIME_ERROR_NullPointerException, NULL);
                //     return;
                // }
                for(int i=target_field->slot_count - 1;i>= 0;i--) {
                    slot_t *field_slot = target_field->slots + i;
                    slot_t *slot = push(frame);
                    slot->bits = field_slot->bits;
                    slot->ref = field_slot->ref;
                }

                if(match_field != NULL) {
                    // 跳过其他的opcode，直接找到调用方法
                    while(1) {
                        skip_instruction(thread);
                        opcode = codes[frame->pc];
                        if(opcode == OPCODE_invokevirtual || opcode == OPCODE_invokeinterface) {
                            u2 high = codes[frame->pc+1];
                            u2 low = codes[frame->pc+2];
                            index = (high << 8) | low;
                            method_t *call_method = resolve_method(thread, class, entries + index);
                            const char *end = strchr(match_field->descriptor, ';');
                            if(call_method != NULL 
                                && strncmp(call_method->klass->class_name, match_field->descriptor + 1, end - match_field->descriptor - 1) == 0) {

                                u2 offset = 0;
                                switch(opcode) {
                                    case OPCODE_invokevirtual:
                                        offset = 3;
                                        break;
                                    case OPCODE_invokeinterface:
                                        offset = 5;
                                        break;
                                }

                                // 设置方法的参数
                                char *ptr = call_method->descriptor + 1;
                                arraylist *args = arraylist_new(2);
                                char buffer[40];
                                while(*ptr && *ptr != ')') {
                                    switch(*ptr) {
                                        case 'I':
                                            arraylist_add(args, "anyInt()");
                                            pop(frame);
                                            break;
                                        case 'J': 
                                            arraylist_add(args, "anyLong()");
                                            pop(frame);
                                            pop(frame);
                                            break;
                                        case 'F': 
                                            arraylist_add(args, "anyFloat()");
                                            pop(frame);
                                            break;
                                        case 'D': 
                                            arraylist_add(args, "anyDouble()");
                                            pop(frame);
                                            pop(frame);
                                            break;
                                        case 'B': 
                                            arraylist_add(args, "anyByte()");
                                            pop(frame);
                                            break;
                                        case 'C': 
                                            arraylist_add(args, "anyChar()");
                                            pop(frame);
                                            break;
                                        case 'S': 
                                            arraylist_add(args, "anyShort()");
                                            pop(frame);
                                            break;
                                        case 'Z': 
                                            arraylist_add(args, "anyBoolean()");
                                            pop(frame);
                                            break;
                                        case 'L': { 
                                            const char *end = strchr(ptr, ';');
                                            const char *start = end - 1;
                                            while(*start != '/') start--;
                                            char *simple_type = strndup(start + 1, end - start - 1);
                                            sprintf(buffer, "any(%s.class)", simple_type);
                                            arraylist_add(args, strdup(buffer));
                                            ptr = strchr(ptr, ';');
                                            pop(frame);
                                            break;
                                        }
                                    }
                                    ptr++;
                                }

                                // pop this
                                pop(frame);

                                // 跳过了方法执行
                                // 判断是否需要创建变量
                                opcode = codes[frame->pc + offset];
                                if(call_method->return_slot_count> 0) {
                                    if(opcode != OPCODE_pop) {
                                        // 有返回值，但不是直接pop，说明新建了变量
                                        if(*ptr == ')') {
                                            ptr++;
                                            var_expr_t *expr_init = var_expr_new(match_field->name);
                                            switch(*ptr) {
                                                case 'I': {
                                                    expr_init->type = "int";
                                                    literal_expr_t *literal = literal_expr_new(LIT_INT);
                                                    literal->l = 1;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'J': {
                                                    expr_init->type = "long";
                                                    literal_expr_t *literal = literal_expr_new(LIT_LONG);
                                                    literal->l = 1L;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'F':  {
                                                    literal_expr_t *literal = literal_expr_new(LIT_FLOAT);
                                                    literal->f = 1.0f;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "float";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'D': {
                                                    literal_expr_t *literal = literal_expr_new(LIT_DOUBLE);
                                                    literal->d = 1.0;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "double";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'B': {
                                                    literal_expr_t *literal = literal_expr_new(LIT_BYTE);
                                                    literal->b = 1;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "byte";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'C': {
                                                    literal_expr_t *literal = literal_expr_new(LIT_CHAR);
                                                    literal->c = '1';
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "char";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'S': {
                                                    literal_expr_t *literal = literal_expr_new(LIT_SHORT);
                                                    literal->i = 1;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "short";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                case 'Z': {
                                                    literal_expr_t *literal = literal_expr_new(LIT_BOOL);
                                                    literal->b = 0;
                                                    expr_t *expr = expr_new(EXPR_LITERAL);
                                                    expr->literal = literal;
                                                    expr_init->type = "boolean";
                                                    expr_init->init = expr;
                                                    break;
                                                }
                                                default: {
                                                    fill_expr_init(expr_init, ptr);
                                                    break;
                                                }
                                            }

                                            // 定义变量
                                            expr_init->name = get_test_method_field_arg(frame->test_method);
                                            expr_t *expr = expr_new(EXPR_VAR);
                                            expr->var = expr_init;
                                            expr_stmt_t *expr_stmt = expr_stmt_new(expr);
                                            stmt_t *stmt = stmt_new(STMT_EXPR);
                                            stmt->expr = expr_stmt;
                                            body_branch_t *branch = calloc(1, sizeof(body_branch_t));
                                            branch->kind = BODY_BRANCH_STMT;
                                            branch->stmt = stmt;
                                            arraylist_add(frame->test_method->branchs, branch);

                                            // todo 将变量保存到test_method中，后续可能需要设置值
                                            test_field_t *local_var = test_field_new(expr_init->name, expr_init->type, ptr);
                                            arraylist_add(frame->test_method->test_local_vars, local_var);

                                            // mock 方法，返回指定的变量
                                            mock_method_call_expr_t *expr_mock_mc = calloc(1, sizeof(mock_method_call_expr_t));
                                            expr_mock_mc->args = args;
                                            expr_mock_mc->type = MOCK_CALL_RETURN;
                                            expr_mock_mc->mock_return = expr_init->name;
                                            expr_mock_mc->field = strdup(match_field->name);
                                            expr_mock_mc->method = strdup(call_method->name);
                                            expr = expr_new(EXPR_MOCK_METHOD_CALL);
                                            expr->mock_method_call = expr_mock_mc;
                                            stmt = stmt_new(STMT_EXPR);
                                            stmt->expr = expr_stmt_new(expr);
                                            branch = calloc(1, sizeof(body_branch_t));
                                            branch->kind = BODY_BRANCH_STMT;
                                            branch->stmt = stmt;
                                            arraylist_add(frame->test_method->branchs, branch);
                                        }
                                    }
                                    for(int i = 0; i < call_method->return_slot_count; i++) {
                                        push(frame);
                                    }
                                }

                                // 跳过这次方法的调用
                                frame->pc += offset;
                                break;
                            }
                        }
                    }
                }

                break;
            }
            case OPCODE_putfield: {   // 0xb5,       // 181
                u1 i1 = codes[frame->pc+1];
                u1 i2 = codes[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                field_t *target_field = resolve_field(thread, entries + index);

                // 从当前堆栈中，pop出指定数量的slot
                slot_t argc[target_field->slot_count];
                index = 0;
                for(u2 i =0;i<target_field->slot_count;i++) {
                    slot_t *slot = pop(frame);
                    argc[i].bits = slot->bits;
                    argc[i].ref = slot->ref;
                }

                // 把slot的信息写到实例对象中
                slot_t *objref = pop(frame);
                object_t *ref = objref->ref;
                for(u2 i=0;i<target_field->slot_count;i++) {
                    slot_t *field_slot = target_field->slots + i;
                    slot_t arg = argc[i];
                    field_slot->bits = arg.bits;
                    field_slot->ref = arg.ref;
                }

                frame->pc += 3;
                break;
            }
            case OPCODE_invokevirtual: {   // 0xb6,       // 182
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                method_t *call_method = resolve_method(thread, class, entries + index);

                // 私有方法需要执行
                if(call_method->access_flags & METHOD_ACC_PRIVATE) {
                    frame_t *run_frame = frame_new(call_method, frame);
                    push_frame(thread, run_frame);
                    return;
                }
                u2 slot_count = call_method->arg_slot_count;
                value_trace_t **args = NULL;
                if(slot_count > 0) {
                    args = calloc(1, sizeof(value_trace_t *));
                    for(u2 i=0;i<slot_count;i++) {
                        slot_t *stack = pop(frame);
                        args[i] = stack->vt;
                    }
                }
                // 把this pop出来
                slot_t *this = pop(frame);
                if(args == NULL && this->vt != NULL && this->vt->kind == VT_INVOKE) {
                    slot_count = 1;
                    args = calloc(1, sizeof(value_trace_t *));
                    args[0] = this->vt;
                }

                // slot_t *stack_slot = pop(frame);
                printf("invokevirtual: %s %s %s\n", call_method->klass->class_name, call_method->name, call_method->descriptor);
                // test_field_t *test_field = stack_slot->test_field;
                // printf("field: %s %s\n", test_field->name, test_field->type);

                // 这里不一定是返回值的，也有可能是上一个方法的返回值被调用
                value_trace_t *vt_invoke = vt_invoke_new(call_method, slot_count, args);
                if(this->test_field) {
                    vt_invoke->invoke.field_name = this->test_field->name;
                    vt_invoke->invoke.call_from_test_field = 1;
                }
                if(call_method->return_slot_count > 0) {
                    slot_t *return_slot = push(frame);
                    return_slot->vt = vt_invoke;
                    for(int i = 0; i < call_method->return_slot_count - 1; i++) {
                        push(frame);
                    }
                }else{
                    this->vt = vt_invoke;
                }
                
                // if(run_method(thread, index, frame, 0) == 1) return;
                break;
            }
            case OPCODE_invokespecial: {   // 0xb7,       // 183
                u2 index1 = codes[frame->pc+1];
                u2 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                method_t *call_method = resolve_method(thread, class, entries + index);

                // 私有方法需要执行
                if(call_method->access_flags & METHOD_ACC_PRIVATE) {
                    frame_t *run_frame = frame_new(call_method, frame);
                    push_frame(thread, run_frame);
                    return;
                }
                u2 slot_count = call_method->arg_slot_count;
                value_trace_t **args = NULL;
                if(slot_count > 0) {
                    args = calloc(1, sizeof(value_trace_t *));
                    for(u2 i=0;i<slot_count;i++) {
                        slot_t *stack = pop(frame);
                        args[i] = stack->vt;
                    }
                }

                // 把this pop出来
                pop(frame);

                // slot_t *stack_slot = pop(frame);
                printf("invokespecial: %s %s %s\n", call_method->klass->class_name, call_method->name, call_method->descriptor);
                // test_field_t *test_field = stack_slot->test_field;
                // printf("field: %s %s\n", test_field->name, test_field->type);
                if(call_method->return_slot_count > 0) {
                    value_trace_t *vt_invoke = vt_invoke_new(call_method, slot_count, args);
                    slot_t *return_slot = push(frame);
                    return_slot->vt = vt_invoke;
                    for(int i = 0; i < call_method->return_slot_count - 1; i++) {
                        push(frame);
                    }
                }
                break;
            }
            case OPCODE_invokestatic: {   // 0xb8,       // 184
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                method_t *call_method = resolve_method(thread, class, entries + index);

                u2 slot_count = call_method->arg_slot_count;
                value_trace_t **args = NULL;
                if(slot_count > 0) {
                    args = calloc(1, sizeof(value_trace_t *));
                    for(u2 i=0;i<slot_count;i++) {
                        slot_t *stack = pop(frame);
                        args[i] = stack->vt;
                    }
                }
                // slot_t *stack_slot = pop(frame);
                printf("invokestatic: %s %s %s\n", call_method->klass->class_name, call_method->name, call_method->descriptor);
                // test_field_t *test_field = stack_slot->test_field;
                // printf("field: %s %s\n", test_field->name, test_field->type);
                if(call_method->return_slot_count > 0) {
                    value_trace_t *vt_invoke = vt_invoke_new(call_method, slot_count, args);
                    slot_t *return_slot = push(frame);
                    return_slot->vt = vt_invoke;
                    for(int i = 0; i < call_method->return_slot_count - 1; i++) {
                        push(frame);
                    }
                }
                // if(run_method(thread, index, frame, 1) == 1) return;
                break;
            }
            case OPCODE_invokeinterface: {   // 0xb9,       // 185
                u2 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                method_t *call_method = resolve_method(thread, class, entries + index);

                u2 slot_count = call_method->arg_slot_count;
                value_trace_t **args = NULL;
                if(slot_count > 0) {
                    args = calloc(1, sizeof(value_trace_t *));
                    for(u2 i=0;i<slot_count;i++) {
                        slot_t *stack = pop(frame);
                        args[i] = stack->vt;
                    }
                }
                
                // 把this pop出来
                pop(frame);

                // slot_t *stack_slot = pop(frame);
                printf("invokeinterface: %s %s %s\n", call_method->klass->class_name, call_method->name, call_method->descriptor);
                // test_field_t *test_field = stack_slot->test_field;
                // printf("field: %s %s\n", test_field->name, test_field->type);
                if(call_method->return_slot_count > 0) {
                    value_trace_t *vt_invoke = vt_invoke_new(call_method, slot_count, args);
                    slot_t *return_slot = push(frame);
                    return_slot->vt = vt_invoke;
                    for(int i = 0; i < call_method->return_slot_count - 1; i++) {
                        push(frame);
                    }
                }
                // if(run_method(thread, index, frame, 0) == 1) return;
                frame->pc += 5;
                break;
            }
            case OPCODE_invokedynamic: {   // 0xba,       // 186
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_new: {   // 0xbb,       // 187
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                rt_cp_entry_t *entry = entries + index;
                object_t *ref = heap_alloc_object(resolve_class(thread, entry));
                push(frame)->ref = ref;
                frame->pc += 3;
                break;
            }
            case OPCODE_newarray: {   // 0xbc,       // 188
                int32_t a_count = pop_int(frame);
                if(a_count < 0) {
                    throw_error(thread, RUNTIME_ERROR_NegativeArraySizeException, NULL);
                    return;
                }
                u1 atype = codes[frame->pc + 1];
                class_t *array_class = load_array_class(thread, atype);
                if(array_class == NULL) {
                    throw_error(thread, RUNTIME_ERROR_NoClassDefFoundError, NULL);
                    return;
                }
                slot_t *slot = push(frame);
                slot->ref = heap_alloc_array(array_class, a_count);
                frame->pc += 2;
                break;
            }
            case OPCODE_anewarray: {   // 0xbd,       // 189
                int32_t count = pop_int(frame);
                if(count < 0) {
                    throw_error(thread, RUNTIME_ERROR_NegativeArraySizeException, NULL);
                    return;
                }
                u1 high = codes[frame->pc + 1];
                u1 low = codes[frame->pc + 2];
                u2 index = (high << 8) | low;
                rt_cp_entry_t *entry = entries + index;
                class_t *target_class = resolve_class(thread, entry);
                slot_t *slot = push(frame);
                slot->ref = heap_alloc_array(target_class, count);
                frame->pc += 3;
                break;
            }
            case OPCODE_arraylength: {   // 0xbe,       // 190
                object_t *ref = pop(frame)->ref;
                if(ref == NULL) {
                    throw_error(thread, RUNTIME_ERROR_NullPointerException, NULL);
                    return;
                }
                push_int(frame, ref->array.length);
                frame->pc++;
                break;
            }
            case OPCODE_athrow: {   // 0xbf,       // 191
                object_t *ref = pop(frame)->ref;
                if(ref == NULL) {
                    throw_error(thread, RUNTIME_ERROR_NullPointerException, NULL);
                    return;
                }
                if(ref->type != OBJ_TYPE_EXCEPTION) {
                    throw_error(thread, RUNTIME_ERROR_RuntimeException, "Invalid exception type");
                    return;
                }
                // todo ref 必须是Throwable的子类
                if(ref->catch_type > 0 && is_class_assignable(ref->klass, load_class("java/lang/Throwable", thread)) == 0) {
                    throw_error(thread, RUNTIME_ERROR_RuntimeException, "must sub class of java/lang/Throwable");
                    return;
                }
                throw_error(thread, RUNTIME_ERROR_RuntimeException, NULL);
                return;
            }
            case OPCODE_checkcast: {   // 0xc0,       // 192
                u1 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                object_t *ref = peek(frame)->ref;
                if(ref != NULL) {
                    class_t *target_class = resolve_class(thread, entries + index);
                    if(is_class_assignable(ref->klass, target_class) == 0) {
                        throw_error(thread, RUNTIME_ERROR_ClassCastException, NULL);
                        return;
                    }
                }

                frame->pc +=3;
                break;
            }
            case OPCODE_instanceof: {   // 0xc1,       // 193
                u1 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                object_t *ref = pop(frame)->ref;
                if(ref == NULL) {
                    push_int(frame, 0);
                }else {
                    class_t *target_class = resolve_class(thread, entries + index);
                    if(is_class_assignable(ref->klass, target_class) == 1) {
                        push_int(frame, 1);
                    }else{
                        push_int(frame, 0);
                    }
                }
                frame->pc += 3;
                break;
            }
            case OPCODE_monitorenter: {   // 0xc2,       // 194
                // todo 暂时不覆盖
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_monitorexit: {   // 0xc3,       // 195
                // todo 暂时不覆盖
                pop(frame);
                frame->pc++;
                break;
            }
            // Control
            case OPCODE_goto: {   // 0xa7,       // 167 
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
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
                int32_t def_val = (int32_t)parse_to_u4(codes + frame->pc + offset);
                offset += 4;
                int32_t low = (int32_t)parse_to_u4(codes + frame->pc + offset);
                offset += 4;
                int32_t high = (int32_t)parse_to_u4(codes + frame->pc + offset);
                u2 length = high - low + 1;
                int32_t index = pop_int(frame);
                if(index < low || index > high) {
                    frame->pc += def_val;
                }else{
                    // jump table
                    offset += 4;
                    offset += (index - low) * 4;
                    int32_t jump_offset = (int32_t)parse_to_u4(codes + frame->pc + offset);
                    frame->pc += jump_offset;
                }
                break;
            }
            case OPCODE_lookupswitch: {   // 0xab,       // 171 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ireturn: {   // 0xac,       // 172 
                pop_frame(thread);
                push_int(thread->current_frame, pop_int(frame));
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
                slot_t *return_slot = pop(frame);
                object_t *ref = return_slot->ref;
                // 有调用者，才把返回值传回去，不然就是最顶层的frame，不需要返回值
                if(frame->invoker) push(frame->invoker)->ref = ref;
                pop_frame(thread);
                return;
            }
            case OPCODE_return: {   // 0xb1,       // 177 
                pop_frame(thread);
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
                object_t *ref = pop(frame)->ref;
                printf("opcode: ifnull     %d\n", ref == NULL ? 1 : 0);
                if(ref != NULL) {
                    frame->pc += 3;
                }else{
                    u1 high = codes[frame->pc+1];
                    u1 low = codes[frame->pc+2];
                    int16_t index = (int16_t)((high << 8) | low);
                    frame->pc += index;
                }
                break;
            }
            case OPCODE_ifnonnull: {   // 0xc7,      // 199 
                object_t *ref = pop(frame)->ref;
                printf("opcode: ifnonnull      %d\n", ref != NULL ? 1 : 0);
                if(ref == NULL) {
                    frame->pc += 3;
                }else{
                    u1 high = codes[frame->pc+1];
                    u1 low = codes[frame->pc+2];
                    int16_t index = (int16_t)((high << 8) | low);
                    frame->pc += index;
                }
                break;
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
