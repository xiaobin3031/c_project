#pragma once

#include "../utils/bytes.h"
#include "../../../core/list/arraylist.h"
#include "method_info.h"
#include "field.h"

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
    field_t **fields;
    u2 methods_count;
    method_t **methods;
    u2 attributes_count;
    void **attributes;

    char *class_name;
} class_t;

u2 slot_count_from_desciptor(char *descriptor);

u2 slot_count_from_class(class_t *class);

class_t *resolve_class(const char *class_name);


class_t *read_class_file(const char *path);

void class_free(class_t *class);