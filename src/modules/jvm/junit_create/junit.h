#pragma once

#include "../../../core/list/arraylist.h"
#include "../utils/bytes.h"
#include "stmt.h"

typedef struct test_method_t test_method_t;

typedef struct {
    char *name;
    char *type;
    arraylist *annos;
} test_field_t;

struct test_method_t {
    char *name;
    char *return_type;
    arraylist *annos;
    arraylist *body;

    // 拼凑变量的后缀
    int local_var_index;
    
    // 实际方法调用
    stmt_t *act_call;
};

typedef struct {
    char *package;
    arraylist *imports;
    arraylist *annos;

    char *class_name;

    arraylist *methods;  // test_method_t 

    arraylist *fields;   // test_field

} test_class_t;

typedef struct {
    // 当前if所在的pc
    u2 pc;

    // 已经处理的if逻辑值，0 进if逻辑，1 进else逻辑，2 跳出逻辑
    int taken;
} if_t;

void create_junit_test_class(
    const char *src_class_dir,
    const char *dest_class_dir,
    const char *new_package_name
);

test_method_t *test_method_new(const char *name, const char *return_type);

test_class_t *test_class_new(const char *package_name, const char *class_name);

test_field_t *test_field_new(const char *name, const char *type);

if_t *if_new(u2 pc);

void print_test_class(test_class_t *test_class);