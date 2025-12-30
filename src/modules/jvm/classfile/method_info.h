#pragma once

#include "../utils/bytes.h"
#include "attr.h"
#include <stdio.h>

typedef enum {
    METHOD_ACC_PUBLIC = 0x0001,
    METHOD_ACC_PRIVATE = 0x0002,
    METHOD_ACC_PROTECTED = 0x0004,
    METHOD_ACC_STATIC = 0x0008,
    METHOD_ACC_FINAL = 0x0010,
    METHOD_ACC_SYNCHRONIZED = 0x0020,
    METHOD_ACC_BRIDGE = 0x0040,
    METHOD_ACC_VARARGS = 0x0080,
    METHOD_ACC_NATIVE = 0x0100,
    METHOD_ACC_ABSTRACT = 0x0400,
    METHOD_ACC_STRICT = 0x0800,
    METHOD_ACC_SYNTHETIC = 0x1000
} method_acc_flags;

typedef struct {
    u2 access_flags;
    char *name;
    char *descriptor;
    u2 attributes_count;
    attribute_t *attributes;

    // 方法入参个数，这两个参数从descriptor中解析,  long/double占两个slot
    u2 arg_slot_count;
} method_t;

int method_is_flag(u2 access_flag, method_acc_flags flag);

method_t *read_methods(FILE *file, u2 method_count, cp_info_t *cp_pools);



void method_free(method_t *methods, u2 method_count);