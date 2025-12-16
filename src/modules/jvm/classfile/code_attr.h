#pragma once

#include "../utils/bytes.h"

typedef struct {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
} exception_table_t;

typedef struct {
    u2 attr_name_index;
    u4 attr_length;
    u2 max_stack;
    u2 max_locals;
    u4 code_length;
    u1 *code;
    u2 exception_table_length;
    exception_table_t **exception_table;
    u2 attributes_count;
    void **attributes;
} code_attr_t;

