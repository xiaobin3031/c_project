#pragma once

#include "../utils/bytes.h"
#include "constant_pool.h"
#include <stdio.h>

typedef enum {
    ATTR_CONSTANT_VALUE = 1,
    ATTR_CODE = 2,
    ATTR_STACK_MAP_TABLE = 3,
    ATTR_EXCEPTIONS = 4,
    ATTR_INNER_CLASSES = 5,
    ATTR_ENCLOSING_METHOD = 6,
    ATTR_SYNTHETIC = 7,
    ATTR_SIGNATURE = 8,
    ATTR_SOURCE_FILE = 9,
    ATTR_SOURCE_DEBUG_EXTENSION = 10,
    ATTR_LINE_NUMBER_TABLE = 11,
    ATTR_LOCAL_VARIABLE_TABLE = 12,
    ATTR_LOCAL_VARIABLE_TYPE_TABLE = 13,
    ATTR_DEPRECATED = 14,
    ATTR_RUNTIME_VISIBLE_ANNOTATIONS = 15,
    ATTR_RUNTIME_INVISIBLE_ANNOTATIONS = 16,
    ATTR_RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS = 17,
    ATTR_RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS = 18,
    ATTR_RUNTIME_VISIBLE_TYPE_ANNOTATIONS = 19,
    ATTR_RUNTIME_INVISIBLE_TYPE_ANNOTATIONS = 20,
    ATTR_ANNOTATION_DEFAULT = 21,
    ATTR_BOOTSTRAP_TABLE = 22,
    ATTR_METHOD_PARAMETERS = 23,
    ATTR_MODULE = 24,
    ATTr_MODULE_PACKAGE = 25,
    ATTR_NEST_HOST = 26,
    ATTR_NEST_MEMBERS = 27,
    ATTR_RECORD = 28,
    ATTR_PERMITTED_SUBCLASSES = 29
} code_attr_tag;

typedef enum {
    VAR_ITEM_TOP = 0,
    VAR_ITEM_INTEGER = 1,
    VAR_ITEM_FLOAT = 2,
    VAR_ITEM_DOUBLE = 3,
    VAR_ITEM_LONG = 4,
    VAR_ITEM_NULL = 5,
    VAR_ITEM_UNINITIALIZED_THIS = 6,
    VAR_ITEM_OBJECT = 7,
    VAR_ITEM_UNINITIALIZED = 8
} variable_info_tag;

typedef struct {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
} exception_table_t;

typedef struct {
    u1 tag;    // 不是class文件中的
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

typedef struct {
    u1 tag;
    union {
        u2 cpool_index;
        u2 offset;
    };
} verification_type_info_t;

typedef struct {
    u1 frame_type;
    u2 offset_delta;
    u2 number_of_locals;
    verification_type_info_t **locals;
    u2 number_of_stack_items;
    verification_type_info_t **stack;
} stack_map_frame_t;

typedef struct {
    u1 tag;
    u2 attr_name_index;
    u4 attr_length;
    u2 number_of_entries;
    stack_map_frame_t **entries;
} stack_map_table_attr_t;

void **read_attributes(FILE *file, u2 attr_count, void **cp_pools);



void attr_free(void **attrs, u2 attr_count);