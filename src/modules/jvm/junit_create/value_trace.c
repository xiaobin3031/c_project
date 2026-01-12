#include "value_trace.h"
#include "../runtime/class.h"
#include "../../../core/list/arraylist.h"
#include <stdlib.h>

static arraylist *g_vt_back_stmts;

value_trace_t *vt_const_new(int64_t value) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_CONST;
    vt->constant.value = value;
    return vt;
}

value_trace_t *vt_field_new(value_trace_t *base, field_t *field) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_FIELD;
    vt->field.base = base;
    vt->field.field = field;
    return vt;
}

value_trace_t *vt_invoke_new(method_t *method, int argc, value_trace_t **args) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_INVOKE;
    vt->invoke.method = method;
    vt->invoke.argc = argc;
    vt->invoke.args = args;
    return vt;
}

value_trace_t *vt_param_new(int index) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_PARAM;
    vt->param.param_index = index;
    return vt;
}

value_trace_t *vt_unknown_new() {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_UNKNOWN;
    return vt;
}

value_trace_t *vt_compare_new(int opcode, value_trace_t *left, value_trace_t *right) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_COMPARE;
    vt->compare.opcode = opcode;
    vt->compare.left = left;
    vt->compare.right = right;
    return vt;
}

value_trace_t *vt_new(vt_kind_e kind) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = kind;
    return vt;
}

void print_value_trace(value_trace_t *vt) {
    if(!vt) return;

    switch(vt->kind) {
        case VT_CONST: {
            printf("vt_const: %f\n", vt->constant.value);
            break;
        }
        case VT_PARAM: {
            printf("vt_param index: %d\n", vt->param.param_index);
            break;
        }
        case VT_FIELD: {
            printf("vt_field: %s %s\n", vt->field.field->name, vt->field.field->descriptor);
            print_value_trace(vt->field.base);
            printf("\n");
            break;
        }
        case VT_INVOKE: {
            printf("vt_invoke: %s %s %s\n", vt->invoke.method->klass->class_name, vt->invoke.method->name, vt->invoke.method->descriptor);
            for(int i=0;i<vt->invoke.argc;i++) {
                printf("arg %d:", i);
                print_value_trace(vt->invoke.args[i]);
            }
            printf("\n");
            break;
        }
        case VT_COMPARE: {
            printf("vt_compare: %d\n", vt->compare.opcode);
            print_value_trace(vt->compare.left);
            print_value_trace(vt->compare.right);
            printf("\n");
            break;
        }
        case VT_UNKNOWN: {
            printf("vt_unknown\n");
            break;
        }
    }
}

void register_vt_back_stmt(const char *class_name, const char *method_name, const char *descriptor, value_trace_back_fn fn) {
    vt_back_stmt_t *stmt = calloc(1, sizeof(vt_back_stmt_t));
    stmt->class_name = class_name;
    stmt->method_name = method_name;
    stmt->descriptor = descriptor;
    stmt->fn = fn;
    arraylist_add(g_vt_back_stmts, stmt);
}

void stringutils_isempty(value_trace_t *vt, test_method_t *method) {

}

void register_vt_back_stmts() {
    if(g_vt_back_stmts == NULL) g_vt_back_stmts = arraylist_new(100);

    register_vt_back_stmt("org/apache/commons/lang3/StringUtils", "isEmpty", "(Ljava/lang/CharSequence;)Z", stringutils_isempty);
}

value_trace_back_fn find_vt_back_fn(const char *class_name, const char *method_name, const char *descriptor) {
    for(size_t i =0;i<g_vt_back_stmts->size;i++) {
        vt_back_stmt_t *stmt = arraylist_get(g_vt_back_stmts, i);
        if(strcmp(stmt->class_name, class_name) == 0 && strcmp(stmt->method_name, method_name) == 0 && strcmp(stmt->descriptor, descriptor) == 0) {
            return stmt->fn;
        }
    }
    return NULL;
}

void value_trace_back_code(value_trace_t *vt, test_method_t *method) {

}