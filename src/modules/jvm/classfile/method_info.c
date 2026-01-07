#include "method_info.h"
#include "class_reader.h"
#include "../utils/bytes.h"
#include "class_bytes.h"
#include "attr.h"
#include <stdlib.h>

int method_is_flag(u2 access_flag, method_acc_flags flag) {
    return (access_flag & flag) == flag ? 1 : 0;
}

method_file_t *read_methods(FILE *file, u2 method_count, cp_info_t *cp_pools) {
    method_file_t *methods = malloc(sizeof(method_file_t) * method_count);
    for (int i = 0; i < method_count; i++) { 
        method_file_t *method = &methods[i];
        method->access_flags = read_u2(file);
        method->name_index = read_u2(file);
        method->descriptor_index = read_u2(file);
        method->attributes_count = read_u2(file);
        method->attributes = read_attributes(file, method->attributes_count, cp_pools);
    }
    return methods;
}

method_file_t *read_methods_bytes(class_file_bytes_t *class_bytes, u2 method_count, cp_info_t *cp_pools) {
    method_file_t *methods = malloc(sizeof(method_file_t) * method_count);
    for (int i = 0; i < method_count; i++) { 
        method_file_t *method = &methods[i];
        method->access_flags = read_bytes_u2(class_bytes);
        method->name_index = read_bytes_u2(class_bytes);
        method->descriptor_index = read_bytes_u2(class_bytes);
        method->attributes_count = read_bytes_u2(class_bytes);
        method->attributes = read_attributes_bytes(class_bytes, method->attributes_count, cp_pools);
    }
    return methods;
}



void method_free(method_file_t *methods, u2 method_count) {
    // todo free
}