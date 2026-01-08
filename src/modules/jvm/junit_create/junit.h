#pragma once

#include "../../../core/list/arraylist.h"

typedef struct {
    char *name;
    char *type;
    arraylist *annos;
} test_field_t;

typedef struct {
    char *name;
    char *return_type;
    arraylist *annos;
    arraylist *body;
} test_method_t;

typedef struct {
    char *package;
    arraylist *imports;

    char *class_name;

    arraylist *methods;  // test_method_t 

    arraylist *fields;   // test_field

} test_class_t;

void create_junit_test_class(
    const char *src_class_dir,
    const char *dest_class_dir,
    const char *new_package_name
);

test_method_t *test_method_new(const char *name, const char *return_type);

test_class_t *test_class_new(const char *package_name, const char *class_name);

test_field_t *test_field_new(const char *name, const char *type);