#pragma once

#include "../../../core/list/arraylist.h"

typedef struct expr_t expr_t;

typedef enum {
    MOCK_CALL_RETURN = 1,
    MOCK_CALL_THROW = 2,
} mock_call_type_e;

typedef enum {
    EXPR_LITERAL = 11,
    EXPR_VAR = 12,
    EXPR_NEW = 13,
    EXPR_METHOD_CALL = 14,
    EXPR_MOCK_METHOD_CALL = 15,
} expr_kind_e;

typedef enum {
    LIT_SHORT = 50,
    LIT_INT = 51,
    LIT_LONG = 52,
    LIT_BOOL = 53,
    LIT_BYTE = 54,
    LIT_STRING = 55,
    LIT_NULL = 56,
    LIT_FLOAT = 57,
    LIT_DOUBLE = 58,
    LIT_CHAR = 59,
} literal_kind_e;

typedef struct {
    const char *type;
    arraylist *args;

    // 有泛型
    int has_params;
} expr_new_t;

typedef struct {
    literal_kind_e kind;

    union {
        int i;
        long l;
        int b;
        char c;
        char *s;
        double d;
        float f;
    };
} literal_expr_t;

typedef struct {
    const char *type;
    const char *name;
    const char *descriptor;
    // 使用的时候初始化
    arraylist *params;

    // 直接用字符串初始化
    expr_t *init;
} var_expr_t;

typedef struct {
    expr_t *expr;       // NULL 表示static调用
    const char *field;
    const char *method;
    arraylist *args;

    // 返回值信息
    const char *res_type;
    const char *res_arg;
} method_call_expr_t;

typedef struct {
    arraylist *args;
    mock_call_type_e type;
    const char *field;
    const char *method;

    union {
        const char *mock_return;
        const char *mock_throw;
    };
} mock_method_call_expr_t;

struct expr_t {

    expr_kind_e kind;
    union {
        literal_expr_t *literal;
        var_expr_t *var;
        method_call_expr_t *method_call;
        mock_method_call_expr_t *mock_method_call;
        expr_new_t *new;
    };
};

literal_expr_t *literal_expr_new(literal_kind_e kind);

expr_new_t *expr_new_new(const char *type, int has_params);

var_expr_t *var_expr_new(const char *name);

method_call_expr_t *method_call_expr_new(expr_t *expr, const char *method);

expr_t *expr_new(expr_kind_e kind);