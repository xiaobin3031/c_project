#pragma once

#include "expr.h"
#include "../../../core/list/arraylist.h"

typedef enum {
    STMT_VAR_DECL,
    STMT_EXPR,
    STMT_ASSERT,
} stmt_kind_e;

typedef struct {
    const char *type;
    const char *name;
    const char *full_type;
    expr_t *init;
} var_decl_stmt_t;

typedef struct {
    expr_t *expr;
} expr_stmt_t;

typedef struct {
    const char *assert_method;  // assertEquals / assertTrue
    arraylist *args;            // expr list
} assert_stmt_t;

typedef struct {
    stmt_kind_e kind;
    union {
        var_decl_stmt_t *var_decl;
        expr_stmt_t *expr;
        assert_stmt_t *assert;
    };
} stmt_t;

var_decl_stmt_t *var_decl_stmt_new(const char *type, const char *name, expr_t *init);

expr_stmt_t *expr_stmt_new(expr_t *expr);

assert_stmt_t *assert_stmt_new(const char *assert_method);

stmt_t *stmt_new(stmt_kind_e kind);

void stmt_free(stmt_t *stmt);