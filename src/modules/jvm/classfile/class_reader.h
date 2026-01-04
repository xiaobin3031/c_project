#pragma once

#include "../utils/bytes.h"
#include "constant_pool.h"
#include "../../../core/list/arraylist.h"
#include "method_info.h"
#include "field.h"
#include <pthread.h>

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

enum class_state {
    CLASS_UNLOADED = 0,
    CLASS_LOADED = 1,
    CLASS_LINKED = 2,
    CLASS_INITING = 3,
    CLASS_INITIALIZED = 4,
    CLASS_ERRONEOUS = 5
};

typedef struct {
    u4 magic;
    u2 minor_version;
    u2 major_version;
    u2 constant_pool_count; 
    cp_info_t *cp_pools;
    u2 access_flags;
    u2 this_class;
    u2 super_class;
    u2 interface_count;
    // u2 **interfaces;
    u2 fields_count;
    field_t *fields;
    u2 methods_count;
    method_t *methods;
    u2 attributes_count;
    attribute_t *attributes;

    char *class_name;

    char *class_simple_name;

    // 属性的slot总数
    u2 total_field_slots;

    pthread_mutex_t lock;
    enum class_state state;

} class_t;

class_t *read_class_file(const char *path);

void class_free(class_t *class);