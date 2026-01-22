#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../utils/slots.h"
#include "../../../core/list/arraylist.h"
#include <string.h>

void add_import(test_class_t *test_class, const char *type) {
    if(strncmp(type, "java.lang.", 10) == 0) {
        return;
    }
    char buffer[1024];
    sprintf(buffer, "import %s;", type);
    for(size_t i = 0; i < test_class->imports->size; i++) {
        char *import = (char*)arraylist_get(test_class->imports, i);
        if(strcmp(import, buffer) == 0) {
            return;
        }
    }
    arraylist_add(test_class->imports, strdup(buffer));
}

test_method_t *test_method_new(const char *name, const char *return_type) {
    test_method_t *method = malloc(sizeof(test_method_t));
    method->name = strdup(name);
    method->return_type = strdup(return_type);
    method->annos = arraylist_new(2);
    method->body = arraylist_new(10);
    method->branchs = arraylist_new(10);
    method->test_local_vars = arraylist_new(5);
    return method;
}

test_class_t *test_class_new(const char *package_name, const char *class_name) {
    test_class_t *test_class = calloc(1, sizeof(test_class_t));
    test_class->package = strdup(package_name);
    test_class->class_name = strdup(class_name);
    test_class->imports = arraylist_new(10);
    test_class->methods = arraylist_new(10);
    test_class->fields = arraylist_new(10);
    test_class->annos = arraylist_new(2);
    return test_class;
}

test_field_t *test_field_new(const char *name, const char *type, const char *descriptor) {
    test_field_t *field = calloc(1, sizeof(test_field_t));
    field->name = strdup(name);
    field->type = strdup(type);
    field->descriptor = strdup(descriptor);
    field->annos = arraylist_new(2);
    return field;
}

if_t *if_new(u2 pc) {
    if_t *if_ = calloc(1, sizeof(if_t));
    if_->pc = pc;
    if_->taken = 0;
    if_->get_pcs[0] = -1;
    if_->get_pcs[1] = -1;
    return if_;
}


char *get_test_method_field_arg(int *arg_index) {
    char buffer[200];
    sprintf(buffer, "localArg%d", *arg_index);
    *arg_index += 1;
    return strdup(buffer);
}

static void fill_expr_init(var_expr_t *expr_init, const char *type, test_class_t *test_class) {
    if(strncmp(type, "Ljava/lang/String;", 18) == 0) {
        expr_init->type = "String";
        literal_expr_t *literal_expr = literal_expr_new(LIT_STRING);
        literal_expr->s = "1";
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strncmp(type, "Ljava/util/List;", 16) == 0) {
        expr_init->type = "List";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("ArrayList", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
        add_import(test_class, "java.util.ArrayList");
    }else if(strncmp(type, "Ljava/util/Map;", 15) == 0) {
        expr_init->type = "Map";
        expr_init->params = arraylist_new(2);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("HashMap", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
        add_import(test_class, "java.util.HashMap");
        add_import(test_class, "java.util.Map");
    }else if(strncmp(type, "Ljava/util/Set;", 15) == 0) {
        expr_init->type = "Set";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "?");
        expr_new_t *exprnew = expr_new_new("HashSet", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
        add_import(test_class, "java.util.HashSet");
        add_import(test_class, "java.util.Set");
    }else if(strncmp(type, "Ljava/lang/Integer;", 19) == 0) {
        expr_init->type = "int";
        literal_expr_t *literal_expr = literal_expr_new(LIT_INT);
        literal_expr->i = 1;
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strncmp(type, "Ljava/lang/Long;", 16) == 0) {
        expr_init->type = "long";
        literal_expr_t *literal_expr = literal_expr_new(LIT_LONG);
        literal_expr->l = 1L;
        expr_t *expr = expr_new(EXPR_LITERAL);
        expr->literal = literal_expr;
        expr_init->init = expr;
    }else if(strncmp(type, "Ljava/math/BigDecimal;", 22) == 0) {
        expr_init->type = "BigDecimal";
        method_call_expr_t *mc = method_call_expr_new(NULL, "BigDecimal.valueOf");
        arraylist_add(mc->args, "1");
        expr_t *expr = expr_new(EXPR_METHOD_CALL);
        expr->method_call = mc;
        expr_init->init = expr;
        add_import(test_class, "java.math.BigDecimal");
    }else if(strncmp(type, "Lorg/redisson/api/RBucket;", 26) == 0) {
        expr_init->type = "RBucket";
        expr_init->params = arraylist_new(1);
        // todo 后续再补充
        arraylist_add(expr_init->params, "Object");
        expr_new_t *exprnew = expr_new_new("MyRBucket", 1);
        expr_t *expr = expr_new(EXPR_NEW);
        expr->new = exprnew;
        expr_init->init = expr;
        add_import(test_class, "org.redisson.api.RBucket");
        add_import(test_class, "com.shanshan.order.juninew.model.MyRBucket");
    }
    else {
        char *tmp_name = strdup(type);
        char *ptr_end = tmp_name;
        if(*ptr_end == 'L') ptr_end++;
        char *ptr = ptr_end;
        while(*ptr && *ptr != ';') {
            if(*ptr == '/') *ptr = '.';
            ptr++;
        }
        if(*ptr == ';') *ptr = '\0';
        add_import(test_class, ptr_end);
        char buffer[200];
        expr_init->type = descriptor_to_simple_type(type);
        sprintf(buffer, "%s.class", expr_init->type);
        method_call_expr_t *mc = method_call_expr_new(NULL, "Util.newAndInit");
        arraylist_add(mc->args, strdup(buffer));
        expr_t *expr = expr_new(EXPR_METHOD_CALL);
        expr->method_call = mc;
        expr_init->init = expr;
        free(tmp_name);
    }
}

stmt_t *init_arg_stmt(char *ptr, test_class_t *test_class, int *arg_index) {
    var_expr_t *expr_init = var_expr_new(NULL);
    switch(*ptr) {
        case 'I': {
            expr_init->type = "int";
            literal_expr_t *literal = literal_expr_new(LIT_INT);
            literal->l = 1;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->init = expr;
            break;
        }
        case 'J': {
            expr_init->type = "long";
            literal_expr_t *literal = literal_expr_new(LIT_LONG);
            literal->l = 1L;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->init = expr;
            break;
        }
        case 'F':  {
            literal_expr_t *literal = literal_expr_new(LIT_FLOAT);
            literal->f = 1.0f;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "float";
            expr_init->init = expr;
            break;
        }
        case 'D': {
            literal_expr_t *literal = literal_expr_new(LIT_DOUBLE);
            literal->d = 1.0;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "double";
            expr_init->init = expr;
            break;
        }
        case 'B': {
            literal_expr_t *literal = literal_expr_new(LIT_BYTE);
            literal->b = 1;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "byte";
            expr_init->init = expr;
            break;
        }
        case 'C': {
            literal_expr_t *literal = literal_expr_new(LIT_CHAR);
            literal->c = '1';
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "char";
            expr_init->init = expr;
            break;
        }
        case 'S': {
            literal_expr_t *literal = literal_expr_new(LIT_SHORT);
            literal->i = 1;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "short";
            expr_init->init = expr;
            break;
        }
        case 'Z': {
            literal_expr_t *literal = literal_expr_new(LIT_BOOL);
            literal->b = 0;
            expr_t *expr = expr_new(EXPR_LITERAL);
            expr->literal = literal;
            expr_init->type = "boolean";
            expr_init->init = expr;
            break;
        }
        default: {
            fill_expr_init(expr_init, ptr, test_class);
            const char *end = strchr(ptr, ';');
            expr_init->descriptor = strndup(ptr, end - ptr + 1);
            break;
        }
    }
    if(expr_init->descriptor == NULL) {
        expr_init->descriptor = strndup(ptr, 1);
    }
    expr_init->name = get_test_method_field_arg(arg_index);
    expr_t *expr = expr_new(EXPR_VAR);
    expr->var = expr_init;
    expr_stmt_t *expr_stmt = expr_stmt_new(expr);
    stmt_t *stmt = stmt_new(STMT_EXPR);
    stmt->expr = expr_stmt;
    return stmt;
}