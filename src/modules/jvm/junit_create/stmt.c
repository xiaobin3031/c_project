#include "stmt.h"
#include "expr.h"
#include "../../../core/list/arraylist.h"
#include <string.h>

var_decl_stmt_t *var_decl_stmt_new( const char *type, const char *name, expr_t *init) {
    var_decl_stmt_t *stmt = calloc(1, sizeof( var_decl_stmt_t));
    stmt->type = strdup(type);
    stmt->name = strdup(name);
    stmt->init = init;
    return stmt;
}

expr_stmt_t *expr_stmt_new( expr_t *expr) {
    expr_stmt_t *stmt = calloc(1, sizeof(expr_stmt_t));
    stmt->expr = expr;
    return stmt;
}

assert_stmt_t *assert_stmt_new(  const char *assert_method) {
    assert_stmt_t *stmt = calloc(1, sizeof( assert_stmt_t));
    stmt->assert_method = strdup( assert_method);
    stmt->args = arraylist_new(2);
    return stmt;
}

stmt_t *stmt_new( stmt_kind_e kind) {
    stmt_t *stmt = calloc(1, sizeof(stmt_t));
    stmt->kind = kind;
    return stmt;
}