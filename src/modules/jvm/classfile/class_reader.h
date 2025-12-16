#pragma once

#include "../utils/bytes.h"
#include "method_info.h"

typedef struct {
    u4 magic;
    u2 minor_version;
    u2 major_version;
    u2 constant_pool_count; 
    void **cp_pools;
    u2 access_flags;
    u2 this_class;
    u2 super_class;
    u2 interface_count;
    // u2 **interfaces;
    u2 fields_count;
    // field_info **fields;
    u2 methods_count;
    method_t **methods;
    // method_info **methods;
    u2 attributes_count;
    // attribute_info **attributes;
} class_t;


class_t *read_class_file(const char *path);

void class_free(class_t *class);