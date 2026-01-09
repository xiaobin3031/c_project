#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../../../core/list/arraylist.h"
#include <string.h>

test_method_t *test_method_new(const char *name, const char *return_type) {
    test_method_t *method = malloc(sizeof(test_method_t));
    method->name = strdup(name);
    method->return_type = strdup(return_type);
    method->annos = arraylist_new(2);
    method->body = arraylist_new(10);
    method->local_var_index = 1;
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

test_field_t *test_field_new(const char *name, const char *type) {
    test_field_t *field = calloc(1, sizeof(test_field_t));
    field->name = strdup(name);
    field->type = strdup(type);
    field->annos = arraylist_new(2);
    return field;
}

if_t *if_new(u2 pc) {
    if_t *if_ = calloc(1, sizeof(if_t));
    if_->pc = pc;
    if_->taken = 0;
    return if_;
}