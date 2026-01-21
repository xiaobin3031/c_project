#pragma once

#include "../../../core/list/arraylist.h"
#include "../utils/bytes.h"
#include "stmt.h"
#include "../project/project.h"

typedef struct test_method_t test_method_t;
typedef struct value_trace_t value_trace_t;
typedef struct if_t if_t;

typedef enum  {
    BODY_BRANCH_IF,
    BODY_BRANCH_STMT,
} body_branch_e;

typedef struct {
    body_branch_e kind;

    union {
        if_t *if_branch;
        stmt_t *stmt;
    };
} body_branch_t;


typedef struct {
    const char *name;
    const char *type;
    const char *descriptor;
    arraylist *annos;
} test_field_t;

struct if_t {
    // 当前if所在的pc
    u2 pc;
    char *if_name;
    // 已经处理的if逻辑值，0 进if逻辑，1 进else逻辑，2 跳出逻辑
    int taken;
    value_trace_t *vt;

    // 走过的pc的记录，防止if中的其他条件再走一次，比如if ( A or B)，这个时候B可以不用覆盖了
    int get_pcs[2];
};

struct test_method_t {
    char *name;
    char *return_type;
    arraylist *annos;
    arraylist *body;

    // 测试方法中的临时变量
    arraylist *test_local_vars;

    // 拼凑变量的后缀
    int local_var_index;
    
    // 实际方法调用
    stmt_t *act_call;

    // 方法的所有节点
    arraylist *all_ifs;

    // 方法节点
    arraylist *branchs;

    // 方法短路，说明本次方法已经跑过了
    int short_circuit;
};

typedef struct {
    char *package;
    arraylist *imports;
    arraylist *annos;

    char *class_name;

    arraylist *methods;  // test_method_t 

    arraylist *fields;   // test_field

} test_class_t;

void create_junit_test_class(
    project_t *project,
    const char *dest_class_dir,
    const char *new_package_name
);

test_method_t *test_method_new(const char *name, const char *return_type);

test_class_t *test_class_new(const char *package_name, const char *class_name);

test_field_t *test_field_new(const char *name, const char *type, const char *descriptor);

if_t *if_new(u2 pc);

void print_test_class(test_class_t *test_class, const char *dest_file_path);

char *get_test_method_field_arg(test_method_t *test_method);