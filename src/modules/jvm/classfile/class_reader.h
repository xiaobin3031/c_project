#pragma once

#include "../utils/bytes.h"
#include "constant_pool.h"
#include "../../../core/list/arraylist.h"
#include "method_info.h"
#include "field.h"

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

    // 属性的slot总数
    u2 total_field_slots;
} class_t;

class_t *read_class_file(const char *path);

void class_free(class_t *class);