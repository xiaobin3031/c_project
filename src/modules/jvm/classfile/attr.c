#include "attr.h"
#include "constant_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int is_attr_tag(void *attr, u1 special_tag) {
    return (u1)*((char*)attr) == special_tag;
}

static verification_type_info_t **read_verification_type_info(FILE *file, u2 length) { 
    if(length == 0) return NULL;
    verification_type_info_t **infos = malloc(length * sizeof(verification_type_info_t *));
    for(u2 i = 0; i < length; i++) {
        infos[i] = malloc(sizeof(verification_type_info_t));
        u1 tag = read_u1(file);
        infos[i]->tag = tag;
        if(tag == VAR_ITEM_OBJECT) {
            infos[i]->cpool_index = read_u2(file);
        }else if(tag == VAR_ITEM_UNINITIALIZED) {
            infos[i]->offset = read_u2(file);
        }
    }
    return infos;
}

void **read_attributes(FILE *file, u2 attr_count, void **cp_pools) {
    if(attr_count <= 0) return NULL;

    void **attrs = malloc(attr_count * sizeof(void *));
    for(u2 i = 0; i < attr_count; i++) {
        u2 attr_name_index = read_u2(file);
        void *info = cp_pools[attr_name_index];
        char *attr_name = get_utf8(info);
        if(strcmp(attr_name, "Code") == 0) {
            code_attr_t *code = malloc(sizeof(code_attr_t));
            code->tag = ATTR_CODE;
            code->attr_name_index = attr_name_index;
            code->attr_length = read_u4(file);
            if(code->attr_length > 0) {
                code->max_stack = read_u2(file);
                code->max_locals = read_u2(file);
                code->code_length = read_u4(file);
                code->code = read_bytes(file, code->code_length);
                code->exception_table_length = read_u2(file);
                code->exception_table = malloc(code->exception_table_length * sizeof(exception_table_t *));
                for(u2 j=0;j<code->exception_table_length;j++) {
                    exception_table_t *exception_table = malloc(sizeof(exception_table_t));
                    exception_table->start_pc = read_u2(file);
                    exception_table->end_pc = read_u2(file);
                    exception_table->handler_pc = read_u2(file);
                    exception_table->catch_type = read_u2(file);
                    code->exception_table[j] = exception_table;
                }
                code->attributes_count = read_u2(file);
                code->attributes = read_attributes(file, code->attributes_count, cp_pools);
            }
            attrs[i] = code;
        } else if(strcmp(attr_name, "StackMapTable") == 0) {
            stack_map_table_attr_t *stack_map_table = malloc(sizeof(stack_map_table_attr_t));
            stack_map_table->tag = ATTR_STACK_MAP_TABLE;
            stack_map_table->attr_name_index = attr_name_index;
            stack_map_table->attr_length = read_u4(file);
            if(stack_map_table->attr_length > 0) {
                stack_map_table->number_of_entries = read_u2(file);
                stack_map_table->entries = malloc(stack_map_table->number_of_entries * sizeof(stack_map_frame_t *));
                for(u2 j=0;j<stack_map_table->number_of_entries;j++) {
                    u1 frame_type = read_u1(file);
                    stack_map_frame_t *stack_map_frame = malloc(sizeof(stack_map_frame_t));
                    stack_map_frame->frame_type = frame_type;
                    if(frame_type >= 64 && frame_type <= 127) {
                        stack_map_frame->number_of_stack_items = 1;
                        stack_map_frame->stack = read_verification_type_info(file, 1);
                    }else if(frame_type == 247) {
                        stack_map_frame->offset_delta = read_u2(file);
                        stack_map_frame->number_of_stack_items = 1;
                        stack_map_frame->stack = read_verification_type_info(file, 1);
                    }else if(frame_type >= 248 && frame_type <= 250) {
                        stack_map_frame->offset_delta = read_u2(file);
                    }else if(frame_type == 251) {
                        stack_map_frame->offset_delta = read_u2(file);
                    }else if(frame_type >= 252 && frame_type <= 254) {
                        stack_map_frame->offset_delta = read_u2(file);
                        stack_map_frame->number_of_locals = frame_type - 251;
                        stack_map_frame->locals = read_verification_type_info(file, stack_map_frame->number_of_locals);
                    }else if(frame_type == 255) {
                        stack_map_frame->offset_delta = read_u2(file);
                        stack_map_frame->number_of_locals = read_u2(file);
                        stack_map_frame->locals = read_verification_type_info(file, stack_map_frame->number_of_locals);
                        stack_map_frame->number_of_stack_items = read_u2(file);
                        stack_map_frame->stack = read_verification_type_info(file, stack_map_frame->number_of_stack_items);
                    }
                    stack_map_table->entries[j] = stack_map_frame;
                }
            }
            attrs[i] = stack_map_table;
        }
        else {
            printf("unknown attribute %s\n", attr_name);
            exit(1);
        }
    }
    return attrs;
}





static void exception_table_free(exception_table_t **exception_table, u2 exception_table_length) {
    if(exception_table) {
        for(u2 i = 0; i < exception_table_length; i++) {
            free(exception_table[i]);
        }
        free(exception_table);
    }
}
static void verification_type_info_free(verification_type_info_t **verification_type_info, u2 number_of_items) {
    if(verification_type_info) {
        for(u2 i = 0; i < number_of_items; i++) {
            free(verification_type_info[i]);
        }
        free(verification_type_info);
    }
}
static void stack_map_frame_free(stack_map_frame_t **stack_map_frame, u2 number_of_entries) {
    if(stack_map_frame) {
        for(u2 i = 0; i < number_of_entries; i++) {
            verification_type_info_free(stack_map_frame[i]->stack, stack_map_frame[i]->number_of_stack_items);
            verification_type_info_free(stack_map_frame[i]->locals, stack_map_frame[i]->number_of_locals);
            free(stack_map_frame[i]);
        }
        free(stack_map_frame);
    }
}

void attr_free(void **attrs, u2 attr_count) {
    if(attrs) {
        for(u2 i = 0; i < attr_count; i++) {
            void *attr = attrs[i];
            if(is_attr_tag(attr, ATTR_CODE)) {
                code_attr_t *code = (code_attr_t *)attr;
                if(code->code) {
                    free(code->code);
                }
                exception_table_free(code->exception_table, code->exception_table_length);
                attr_free(code->attributes, code->attributes_count);
            }else if(is_attr_tag(attr, ATTR_STACK_MAP_TABLE)) {
                stack_map_table_attr_t *stack_map_table = (stack_map_table_attr_t *)attr;
                stack_map_frame_free(stack_map_table->entries, stack_map_table->number_of_entries);
            }
            free(attrs[i]);
        }
        free(attrs);
    }
}