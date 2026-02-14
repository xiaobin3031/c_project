#include "interpreter.h"
#include "opcode.h"
#include "../runtime/frame.h"
#include "../runtime/operand_stack.h"
#include "../runtime/local_vars.h"
#include "../runtime/class.h"
#include "../utils/slots.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * 这个文件只是用来跳过某些指令
 */

void skip_get_field(frame_t *frame, rt_cp_entry_t *entry) { 
    u2 slot_count = 0;
    if(entry->resolved == 1) {
        field_t *field = entry->field;
        slot_count = field->slot_count;
    }else{
        slot_count = slot_count_from_desciptor(entry->sym.descriptor);
    }
    for(int i=slot_count - 1;i>= 0;i--) {
        push(frame);
    }
}

void skip_put_field(frame_t *frame, rt_cp_entry_t *entry) { 
    u2 slot_count = 0;
    if(entry->resolved == 1) {
        field_t *field = entry->field;
        slot_count = field->slot_count;
    }else{
        slot_count = slot_count_from_desciptor(entry->sym.descriptor);
    }
    for(int i=slot_count - 1;i>= 0;i--) {
        pop(frame);
    }
}

void skip_method(frame_t *frame, rt_cp_entry_t *entry) {
    u2 slot_count = 0;
    u2 return_slot_count = 0;
    if(entry->resolved == 1) {
        method_t *method = entry->method;
        slot_count = method->arg_slot_count;
        return_slot_count = method->return_slot_count;
    }else{
        slot_count = slot_count_from_desciptor(entry->sym.descriptor);
        char *ptr = entry->sym.descriptor;
        while(*ptr && *ptr != ')') ptr++;
        if(*ptr && *ptr == ')') ptr++;
        return_slot_count = slot_count_from_desciptor(ptr);
    }
    for(int i=slot_count - 1;i>= 0;i--) {
        pop(frame);
    }
    for(int i=return_slot_count - 1;i>= 0;i--) {
        push(frame);
    }

}


/**
 * 只跳过一次
 */
void skip_instruction(jvm_thread_t *thread) {
    frame_t *frame = thread->current_frame;
    u4 code_length = frame->method->code->code_length;
    if(code_length == 0) return;

    u1 *codes = frame->method->code->codes;
    u1 opcode;
    class_t *class = frame->method->klass;
    rt_cp_entry_t *entries = class->entries;

    if(frame->pc < code_length) {
        opcode = codes[frame->pc];
        printf("skip  =======  %d: opcode: %d %s\n", frame->pc, opcode, opcode_to_string(opcode));
        switch(opcode) {
            // Constants
            case OPCODE_nop: {   // 0x00,  // 00 
                frame->pc++;
                break;
            }
            case OPCODE_aconst_null: {   // 0x01,  // 01 
                push(frame);
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
                push(frame);
                frame->pc += 2;
                break;
            }
            case OPCODE_sipush: {   // 0x11,  // 17 
                push(frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_ldc: {   // 0x12,  // 18 
                u1 index = codes[frame->pc+1];
                rt_cp_entry_t *entry = entries + index;
                // printf("ldc cp_info.tag: %d\n", cp_info.tag);
                if(entry->tag == RT_CONSTANT_Integer) {
                    push(frame);
                }else if(entry->tag == RT_CONSTANT_Float) {
                    push(frame);
                }else if(entry->tag == RT_CONSTANT_String) {
                    push(frame);
                }else if(entry->tag == RT_CONSTANT_Class) {
                    push(frame);
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
                    push(frame);
                    push(frame);
                }else if(entry->tag == RT_CONSTANT_Double) {
                    push(frame);
                    push(frame);
                }else {
                    throw_error(thread, RUNTIME_ERROR_RuntimeException, "ldc2_w tag error, not long or double");
                }
                frame->pc += 3;
                break;
            }
            // Loads
            case OPCODE_iload: {   // 0x15,     // 21 
                u1 index = codes[frame->pc+1];
                push(frame);
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
                push(frame);
                frame->pc += 2;
                break;
            }
            case OPCODE_iload_0: {   // 0x1a,     // 26 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iload_1: {   // 0x1b,     // 27 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iload_2: {   // 0x1c,     // 28 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iload_3: {   // 0x1d,     // 29 
                push(frame);
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
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_aload_1: {   // 0x2b,     // 43 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_aload_2: {   // 0x2c,     // 44 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_aload_3: {   // 0x2d,     // 45 
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iaload: {   // 0x2e,     // 46 
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
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
                pop(frame);
                frame->pc += 2;
                break;
            }
            case OPCODE_istore_0: {   // 0x3b,       // 59 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_istore_1: {   // 0x3c,       // 60 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_istore_2: {   // 0x3d,       // 61 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_istore_3: {   // 0x3e,       // 62 
                pop(frame);
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
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_astore_1: {   // 0x4c,       // 76 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_astore_2: {   // 0x4d,       // 77 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_astore_3: {   // 0x4e,       // 78 
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_iastore: {   // 0x4f,       // 79 
                pop(frame);
                pop(frame);
                pop(frame);
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
                pop(frame);
                pop(frame);
                pop(frame);
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
                pop(frame);
                push(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
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
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_lshl: {   // 0x79,       // 121 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ishr: {   // 0x7a,       // 122 
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_lshr: {   // 0x7b,       // 123 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iushr: {   // 0x7c,       // 124 
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_lushr: {   // 0x7d,       // 125 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iand: {   // 0x7e,       // 126 
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_land: {   // 0x7f,       // 127 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ior: {   // 0x80,       // 128 
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_lor: {   // 0x81,       // 129 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ixor: {   // 0x82,       // 130 
                pop(frame);
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_lxor: {   // 0x83,       // 131 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_iinc: {   // 0x84,       // 132 
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
            // References
            case OPCODE_getstatic: {   // 0xb2,       // 178
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                skip_get_field(frame, entries + index);
                frame->pc += 3;
                break;
            }
            case OPCODE_putstatic: {   // 0xb3,       // 179
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                skip_put_field(frame, entries + index);
                frame->pc += 3;
                break;
            }
            case OPCODE_getfield: {   // 0xb4,       // 180
                u1 i1 = codes[frame->pc+1];
                u1 i2 = codes[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                pop(frame);
                skip_get_field(frame, entries + index);

                frame->pc += 3;
                break;
            }
            case OPCODE_putfield: {   // 0xb5,       // 181
                u1 i1 = codes[frame->pc+1];
                u1 i2 = codes[frame->pc+2];
                u2 index = (i1 << 8) | i2;
                skip_put_field(frame, entries + index);
                pop(frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_invokevirtual: {   // 0xb6,       // 182
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                pop(frame);
                skip_method(frame, entries + index);
                break;
            }
            case OPCODE_invokespecial: {   // 0xb7,       // 183
                u2 index1 = codes[frame->pc+1];
                u2 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                pop(frame);
                skip_method(frame, entries + index);
                break;
            }
            case OPCODE_invokestatic: {   // 0xb8,       // 184
                u1 index1 = codes[frame->pc+1];
                u1 index2 = codes[frame->pc+2];
                u2 index = (index1 << 8) | index2;
                frame->pc += 3;
                skip_method(frame, entries + index);
                break;
            }
            case OPCODE_invokeinterface: {   // 0xb9,       // 185
                u2 high = codes[frame->pc+1];
                u1 low = codes[frame->pc+2];
                u2 index = (high << 8) | low;
                pop(frame);
                skip_method(frame, entries + index);
                frame->pc += 5;
                break;
            }
            case OPCODE_invokedynamic: {   // 0xba,       // 186
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_new: {   // 0xbb,       // 187
                push(frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_newarray: {   // 0xbc,       // 188
                pop(frame);
                push(frame);
                frame->pc += 2;
                break;
            }
            case OPCODE_anewarray: {   // 0xbd,       // 189
                pop(frame);
                push(frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_arraylength: {   // 0xbe,       // 190
                pop(frame);
                push(frame);
                frame->pc++;
                break;
            }
            case OPCODE_checkcast: {   // 0xc0,       // 192
                frame->pc +=3;
                break;
            }
            case OPCODE_instanceof: {   // 0xc1,       // 193
                push(frame);
                frame->pc += 3;
                break;
            }
            case OPCODE_monitorenter: {   // 0xc2,       // 194
                pop(frame);
                frame->pc++;
                break;
            }
            case OPCODE_monitorexit: {   // 0xc3,       // 195
                pop(frame);
                frame->pc++;
                break;
            }
            // Control
            case OPCODE_jsr: {   // 0xa8,       // 168 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_ret: {   // 0xa9,       // 169 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_lookupswitch: {   // 0xab,       // 171 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
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
            // Extended
            case OPCODE_wide: {   // 0xc4,      // 196 
            fprintf(stderr, "unimpleted opcode: %d\n", opcode);
                            abort();
            }
            case OPCODE_multianewarray: {   // 0xc5,      // 197 
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
                fprintf(stderr, "can not skip opcode: %d\n", opcode);
                abort();
        }
    }
}
