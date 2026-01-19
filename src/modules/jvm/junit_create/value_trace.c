#include "value_trace.h"
#include "../runtime/class.h"
#include "../../../core/list/arraylist.h"
#include "../interpreter/opcode.h"
#include <stdlib.h>
#include <string.h>

static arraylist *g_vt_back_stmts;

value_trace_t *vt_const_new(int64_t value) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_CONST;
    vt->constant.value = value;
    return vt;
}

value_trace_t *vt_string_new(const char *string) {
    value_trace_t *vt = calloc(1, sizeof(value_trace_t));
    vt->kind = VT_STRING;
    vt->string.value = strdup(string);
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

void print_value_trace(value_trace_t *vt, int depth) {
    if (!vt) {
        printf("%*s(null)\n", depth * 2, "");
        return;
    }

    const char *indent = "";
    int indent_width = depth * 2;

    switch (vt->kind) {
        case VT_CONST: {
            switch(vt->constant.sub_kind) {
                case VT_SUB_CONST_INT:
                    printf("%*svt_const: %d\n", indent_width, indent, (int32_t)vt->constant.value);
                    break;
                case VT_SUB_CONST_LONG:
                    printf("%*svt_const: %lld\n", indent_width, indent, (int64_t)vt->constant.value);
                    break;
            }
            break;
        }

        case VT_PARAM: {
            printf("%*svt_param: index=%d\n",
                   indent_width, indent, vt->param.param_index);
            break;
        }

        case VT_FIELD: {
            printf("%*svt_field: %s %s\n",
                   indent_width, indent,
                   vt->field.field->name,
                   vt->field.field->descriptor);

            printf("%*sbase:\n", indent_width + 2, indent);
            print_value_trace(vt->field.base, depth + 2);
            break;
        }

        case VT_INVOKE: {
            printf("%*svt_invoke: %s %s %s\n",
                   indent_width, indent,
                   vt->invoke.method->klass->class_name,
                   vt->invoke.method->name,
                   vt->invoke.method->descriptor);

            for (int i = 0; i < vt->invoke.argc; i++) {
                printf("%*sarg[%d]:\n", indent_width + 2, indent, i);
                print_value_trace(vt->invoke.args[i], depth + 2);
            }
            break;
        }

        case VT_COMPARE: {
            printf("%*svt_compare: opcode=%d\n",
                   indent_width, indent, vt->compare.opcode);

            printf("%*sleft:\n", indent_width + 2, indent);
            print_value_trace(vt->compare.left, depth + 2);

            printf("%*sright:\n", indent_width + 2, indent);
            print_value_trace(vt->compare.right, depth + 2);
            break;
        }

        case VT_UNKNOWN: {
            printf("%*svt_unknown\n", indent_width, indent);
            break;
        }
    }
}


void register_vt_back_stmt(const char *class_name, const char *method_name, const char *descriptor, value_trace_back_fn fn) {
    vt_back_stmt_t *stmt = calloc(1, sizeof(vt_back_stmt_t));
    stmt->class_name = strdup(class_name);
    stmt->method_name = strdup(method_name);
    stmt->descriptor = strdup(descriptor);
    stmt->fn = fn;
    arraylist_add(g_vt_back_stmts, stmt);
}

value_trace_t *stringutils_isempty(value_trace_t *vt, test_method_t *method) {
    value_trace_t *value = vt_new(VT_UNKNOWN);
    value_trace_t *next = vt->invoke.args[0];
    if(vt->value == NULL) {
        perror("StringUtils is empty value is null");
        abort();
    }
    if(vt->value->constant.value == 0) {
        // 要不为空
        value->kind = VT_STRING;
        // 方便使用的时候直接使用
        value->string.value = "\"1\"";
    }else{
        // 要为空
        value->kind = VT_NULL;
    }
    next->value = value;
    return next;
}

value_trace_t *string_length(value_trace_t *vt, test_method_t *method) {
    value_trace_t *value = vt_new(VT_UNKNOWN);
    value_trace_t *next = vt->invoke.args[0];
    if(vt->value == NULL) {
        perror("String length value is null");
        abort();
    }
    value->kind = VT_STRING;
    char buffer[100];
    sprintf(buffer, "Util.randomStringByLen(%d)", (int32_t)vt->value->constant.value);
    value->string.value = strdup(buffer);
    next->value = value;
    return next;
}

void register_vt_back_stmts() {
    if(g_vt_back_stmts == NULL) g_vt_back_stmts = arraylist_new(100);

    register_vt_back_stmt("org/apache/commons/lang3/StringUtils", "isEmpty", "(Ljava/lang/CharSequence;)Z", stringutils_isempty);
    register_vt_back_stmt("java/lang/String", "length", "()I", string_length);
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

char *get_value(value_trace_t *value) {
    switch(value->kind) {
        case VT_NULL:
            return "null";
        case VT_STRING:
            return value->string.value;
    }
    return "null";
}

/**
 * value_trace_t 是否是常量，不一定是常量，null也算是
 */
int is_vt_const(value_trace_t *vt) {
    return vt->kind == VT_CONST;
}

int is_expr_equal(expr_t *left, expr_t *right) {
    if(left->kind == right->kind) {
        switch(left->kind) {
            case EXPR_METHOD_CALL: {
                int equal = strcmp(left->method_call->method, right->method_call->method);
                if(equal == 0) {
                    equal = (left->method_call->field == NULL && right->method_call->field == NULL)
                        || strcmp(left->method_call->field, right->method_call->field) == 0;
                }
                return equal;
            }
        }
    }
    return 0;
}

void add_test_method_body(arraylist *body, stmt_t *stmt) {
    // 需要去重
    for(size_t i=0;i<body->size;i++) {
        stmt_t *tmp = arraylist_get(body, i);
        if(tmp->kind == stmt->kind) {
            switch(tmp->kind) {
                case STMT_EXPR: {
                    expr_t *tmp_expr = tmp->expr->expr;
                    expr_t *stmt_expr = stmt->expr->expr;
                    if(is_expr_equal(tmp->expr->expr, stmt->expr->expr) == 1) {
                        stmt_t *old = (stmt_t*)arraylist_set(body, i, stmt);
                        stmt_free(old);
                        return;
                    }
                    break;
                }
            }
        }
    }
    arraylist_add(body, stmt);
}

void value_trace_back_code(value_trace_t *vt, test_method_t *test_method) {
    if(!vt) return;

    if(g_vt_back_stmts == NULL) register_vt_back_stmts();

    switch(vt->kind) {
        case VT_INVOKE: {
            value_trace_back_fn fn = find_vt_back_fn(vt->invoke.method->klass->class_name, vt->invoke.method->name, vt->invoke.method->descriptor);
            if(fn != NULL) {
                value_trace_t *next = fn(vt, test_method);
                value_trace_back_code(next, test_method);
            } else if(vt->invoke.call_from_test_field == 1) {
                // 调用了test_field的方法，需要设置
                method_t *method = vt->invoke.method;
                char *method_name = strdup(method->name);
                if(strncmp(method_name, "get", 3) == 0) {
                    method_name[0] = 's';
                    method_call_expr_t *mc_expr = method_call_expr_new(NULL, method_name);
                    mc_expr->field = strdup(vt->invoke.field_name);
                    arraylist_add(mc_expr->args, get_value(vt->value));

                    expr_t *expr = expr_new(EXPR_METHOD_CALL);
                    expr->method_call = mc_expr;
                    expr_stmt_t *stmt_expr = expr_stmt_new(expr);

                    stmt_t *stmt = stmt_new(STMT_EXPR);
                    stmt->expr = stmt_expr;
                    add_test_method_body(test_method->body, stmt);
                }
                else {
                    printf("unknown back code, invoke: %s\n", method_name);
                }

                free(method_name);
            }
            break;
        }
        case VT_COMPARE: {
            value_trace_t *left_vt = vt->compare.left;
            value_trace_t *right_vt = vt->compare.right;


            // 一个作为变量，一个作为值
            value_trace_t *call = NULL;
            value_trace_t *value = NULL;

            if(is_vt_const(left_vt)) {
                value = left_vt;
                call = right_vt;
            }else if(is_vt_const(right_vt)){
                value = right_vt;
                call = left_vt;
            }else{
                // 都不是常量，只能把其中一个解析成常量
                printf("unknown back code, compare: %s, left-kind: %d, right-kind: %d\n", opcode_to_string(vt->compare.opcode), left_vt->kind, right_vt->kind);
                // 强制需要覆盖
                abort();
            }

            // 再设置value的值
            if(vt->compare.opcode == OPCODE_if_icmpeq) {
                if(value->constant.value != 0) {
                    value->constant.value = value->constant.value + 1;
                }
            }else{
                printf("unknown back code, compare: %s\n", opcode_to_string(vt->compare.opcode));
                // 强制需要覆盖
                abort();
            }
            call->value = value;
            value_trace_back_code(call, test_method);

            break;
        }
    }
}