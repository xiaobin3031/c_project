#pragma once

#include "../../../core/list/arraylist.h"

typedef struct expr_t expr_t;

typedef enum {
    EXPR_LITERAL,
    EXPR_VAR,
    EXPR_NEW,
    EXPR_METHOD_CALL,
    EXPR_MOCK_METHOD_CALL,
} expr_kind_e;

typedef enum {
    LIT_SHORT,
    LIT_INT,
    LIT_LONG,
    LIT_BOOL,
    LIT_BYTE,
    LIT_STRING,
    LIT_NULL,
    LIT_FLOAT,
    LIT_DOUBLE,
    LIT_CHAR,
} literal_kind_e;

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
    char *type;
    char *name;

    // 直接用字符串初始化
    const char *init;
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
    };
};

literal_expr_t *literal_expr_new(literal_kind_e kind);

var_expr_t *var_expr_new(const char *name);

method_call_expr_t *method_call_expr_new(expr_t *expr, const char *method);

expr_t *expr_new(expr_kind_e kind);