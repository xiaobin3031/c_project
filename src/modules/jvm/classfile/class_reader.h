#pragma once

#include "../utils/bytes.h"
#include "constant_pool.h"
#include "../../../core/list/arraylist.h"
#include "method_info.h"
#include "field.h"
#include "class_bytes.h"
#include <pthread.h>

typedef struct class_file_t class_file_t;

enum class_acc_flags {
    CLASS_ACC_PUBLIC = 0x0001,
    CLASS_ACC_FINAL = 0x0010,
    CLASS_ACC_SUPER = 0x0020,
    CLASS_ACC_INTERFACE = 0x0200,
    CLASS_ACC_ABSTRACT = 0x0400,
    CLASS_ACC_SYNTHETIC = 0x1000,
    CLASS_ACC_ANNOTATION = 0x2000,
    CLASS_ACC_ENUM = 0x4000,
    CLASS_ACC_MODULE = 0x8000
};

struct class_file_t {
    u4 magic;
    u2 minor_version;
    u2 major_version;
    u2 constant_pool_count; 
    cp_info_t *cp_pools;
    u2 access_flags;
    u2 this_class;
    u2 super_class;
    u2 interface_count;
    u2 *interfaces;
    u2 fields_count;
    field_file_t *fields;
    u2 methods_count;
    method_file_t *methods;
    u2 attributes_count;
    attribute_file_t *attributes;
};

class_file_t *read_class_file(const char *path);

class_file_t *read_by_class_bytes(class_file_bytes_t *class_bytes);

void class_free(class_file_t *class);

int is_class(class_file_t *class);