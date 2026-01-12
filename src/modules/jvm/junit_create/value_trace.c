#include "value_trace.h"
#include "../runtime/class.h"
#include <stdlib.h>

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