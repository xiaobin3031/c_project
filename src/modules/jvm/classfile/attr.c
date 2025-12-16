#include "attr.h"
#include "constant_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int is_attr_tag(void *attr, u1 special_tag) {
    return (u1)*((char*)attr) == special_tag;
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
        }
    }
    return attrs;
}





void exception_table_free(exception_table_t **exception_table, u2 exception_table_length) {
    if(exception_table) {
        for(u2 i = 0; i < exception_table_length; i++) {
            free(exception_table[i]);
        }
        free(exception_table);
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
            }
            free(attrs[i]);
        }
        free(attrs);
    }
}