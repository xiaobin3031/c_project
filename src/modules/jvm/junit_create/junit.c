#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../../../core/list/arraylist.h"
#include <string.h>

test_method_t *test_method_new(const char *name) {
    test_method_t *method = malloc(sizeof(test_method_t));
    method->name = name;
    method->annos = arraylist_new(2);
    method->body = arraylist_new(10);
    return method;
}

test_method_t *test_class_new(const char *package_name, const char *class_name) {
    test_class_t *test_class = calloc(1, sizeof(test_class_t));
    test_class->package = strdup(package_name);
    test_class->class_name = strdup(class_name);
    test_class->imports = arraylist_new(10);
    test_class->methods = arraylist_new(10);
    test_class->fields = arraylist_new(10);
    return test_class;
}

test_field_t *test_field_new(const char *name, const char *type) {
    test_field_t *field = calloc(1, sizeof(test_field_t));
    field->name = strdup(name);
    field->type = strdup(type);
    field->annos = arraylist_new(2);
    return field;
}