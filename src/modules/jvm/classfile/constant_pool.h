#pragma once

#include "../utils/bytes.h"
#include <stdio.h>

typedef enum {
    CONSTANT_Utf8 = 1,

    CONSTANT_Integer = 3,
    CONSTANT_Float = 4,
    CONSTANT_Long = 5,
    CONSTANT_Double = 6,
    CONSTANT_Class = 7,
    CONSTANT_String = 8,
    CONSTANT_Fieldref = 9,
    CONSTANT_Methodref = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType = 12,

    CONSTANT_MethodHandle = 15,
    CONSTANT_MethodType = 16,
    CONSTANT_Dynamic = 17,
    CONSTANT_InvokeDynamic = 18,
    CONSTANT_Module = 19,
    CONSTANT_Package = 20
} cp_tag;

typedef struct {
    u1 tag;
    u2 length;
    u1 *bytes;
} cp_utf8_t;

typedef struct {
    u1 tag;
    u4 bytes;
} cp_integer_t;

typedef struct {
    u1 tag;
    u2 name_index;
} cp_class_t;

typedef struct {
    u1 tag;
    u2 name_index;
    u2 descriptor_index;
} cp_nameandtype_t;

typedef struct {
    u1 tag;
    u2 class_index;
    u2 name_and_type_index;
} cp_methodref_t;

typedef struct {
    u1 tag;
    u2 class_index;
    u2 name_and_type_index;
} cp_fieldref_t;


int is_cp_info_tag(void *cp_info, u1 special_tag);
void check_cp_info_tag(void *cp_info, u1 special_tag);

void **read_constant_pool(FILE *file, u2 pool_size);

char *get_utf8(void *cp_info);


void print_class_name(void **cp_pools, u2 pool_size);
void print_method_name(void **cp_pools, u2 pool_size);

void constant_pool_free(void **cp_pool, u2 pool_size);