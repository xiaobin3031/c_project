#include "expr.h"
#include "../../../core/list/arraylist.h"
#include <string.h>

literal_expr_t *literal_expr_new(literal_kind_e kind) {
    literal_expr_t *expr = calloc(1, sizeof(literal_expr_t));
    expr->kind = kind;
    return expr;
}

expr_new_t *expr_new_new(const char *type, int has_params) {
    expr_new_t *expr = calloc(1, sizeof(expr_new_t));
    if(type) expr->type = strdup(type);
    expr->has_params = has_params;
    return expr;
}

var_expr_t *var_expr_new(const char *name) {
    var_expr_t *expr = calloc(1, sizeof(var_expr_t));
    if(name) expr->name = strdup(name);
    return expr;
}

method_call_expr_t *method_call_expr_new(expr_t *expr, const char *method) {
    method_call_expr_t *expr_method_call = calloc(1, sizeof(method_call_expr_t));
    expr_method_call->expr = expr;
    if(method) expr_method_call->method = strdup(method);
    expr_method_call->args = arraylist_new(2);
    return expr_method_call;
}

expr_t *expr_new(expr_kind_e kind) {
    expr_t *expr = calloc(1, sizeof(expr_t));
    expr->kind = kind;
    return expr;
}