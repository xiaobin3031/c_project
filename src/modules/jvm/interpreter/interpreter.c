#include "interpreter.h"
#include "opcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void interpret(frame_t *frame, void **cp_pools) {
    u1 opcode;
    int sp = -1, pc = 0;

    while(pc < frame->code_length) {
        opcode = frame->code[pc++];
        switch(opcode) {
            case OPCODE_iconst_1:
                frame->operand_stack[++sp] = 1;
                break;
            case OPCODE_iconst_2:
                frame->operand_stack[++sp] = 2;
                break;
            case OPCODE_istore_1:
                frame->local_vars[1] = frame->operand_stack[sp--];
                break;
            case OPCODE_istore_2:
                frame->local_vars[2] = frame->operand_stack[sp--];
                break;
            case OPCODE_istore_3:
                frame->local_vars[3] = frame->operand_stack[sp--];
                break;
            case OPCODE_iload_1:
                frame->operand_stack[++sp] = frame->local_vars[1];
                break;
            case OPCODE_iload_2:
                frame->operand_stack[++sp] = frame->local_vars[2];
                break;
            case OPCODE_iload_3: 
                frame->operand_stack[++sp] = frame->local_vars[3];
                break;
            case OPCODE_iadd: {
                int v2 = frame->operand_stack[sp--];
                int v1 = frame->operand_stack[sp--];
                int r = v1 + v2;
                frame->operand_stack[++sp] = r;
                break;
            }
            case OPCODE_getstatic: {
                u1 index1 = frame->code[pc++];
                u1 index2 = frame->code[pc++];
                u2 index = (index1 << 8) | index2;
                printf("[WARN] getstatic #%d ignored.\n", index);
                break;
            }
            case OPCODE_invokevirtual: {
                u1 index1 = frame->code[pc++];
                u1 index2 = frame->code[pc++];
                u2 index = (index1 << 8) | index2;
                void *info = cp_pools[index];
                check_cp_info_tag(info, CONSTANT_Methodref);
                cp_methodref_t *methodref = (cp_methodref_t *)info;
                info = cp_pools[methodref->name_and_type_index];
                check_cp_info_tag(info, CONSTANT_NameAndType);
                cp_nameandtype_t *nameandtype = (cp_nameandtype_t *)info;
                char *name = get_utf8(cp_pools[nameandtype->name_index]);
                printf("method_name: %s\n", name);
                int arg = frame->operand_stack[sp--];
                int obj = frame->operand_stack[sp--];
                // todo 暂时用不到
                (void) obj;
                if(strcmp(name, "println") == 0) {
                    printf("%d\n", arg);
                }
                break;
            }
            case OPCODE_return:
                break;
            default: 
                fprintf(stderr, "unknown opcode: %d\n", opcode);
                exit(1);
        }
    }
}