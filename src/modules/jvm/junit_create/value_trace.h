#pragma once

#include <stdlib.h>
#include "../runtime/class.h"

typedef void (*value_trace_back_fn)(value_trace_t *vt, test_method_t *method);

typedef struct field_t field_t;
typedef struct method_t method_t;
typedef struct value_trace_t value_trace_t;
typedef struct test_method_t test_method_t;
typedef struct vt_back_stmt_t vt_back_stmt_t;

typedef enum {
    VT_CONST,   // 常量
    VT_PARAM,
    VT_FIELD,
    VT_INVOKE,
    VT_COMPARE,

    VT_UNKNOWN
} vt_kind_e;

struct vt_back_stmt_t {
    char *class_name;
    char *method_name;
    char *descriptor;

    value_trace_back_fn fn;
};

struct value_trace_t { 
    vt_kind_e kind;

    union {

        // 常量
        struct {
            int64_t  value;
        } constant;

        // 参数
        struct {
            int param_index;
        } param;

        // 字段
        struct {
            value_trace_t *base;
            field_t *field;
        } field;

        // 方法调用
        struct {
            method_t *method;
            int argc;
            value_trace_t **args;
        } invoke;

        // 比较if
        struct {
            int opcode;
            value_trace_t *left;
            value_trace_t *right;
        } compare;
    };
};

value_trace_t *vt_const_new(int64_t value);

value_trace_t *vt_field_new(value_trace_t *base, field_t *field);

value_trace_t *vt_invoke_new(method_t *method, int argc, value_trace_t **args);

value_trace_t *vt_param_new(int index);

value_trace_t *vt_unknown_new();

value_trace_t *vt_compare_new(int opcode, value_trace_t *left, value_trace_t *right);

value_trace_t *vt_new(vt_kind_e kind);

void print_value_trace(value_trace_t *vt);

void register_vt_back_stmts();

void value_trace_back_code(value_trace_t *vt, test_method_t *method);