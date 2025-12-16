#include "method_info.h"
#include "attr.h"
#include <stdlib.h>

method_t **read_methods(FILE *file, u2 method_count, void **cp_pools) {
    method_t **methods = malloc(sizeof(method_t *) * method_count);
    for (int i = 0; i < method_count; i++) { 
        method_t *method = malloc(sizeof(method_t));
        method->access_flags = read_u2(file);
        method->name_index = read_u2(file);
        method->descriptor_index = read_u2(file);
        method->attributes_count = read_u2(file);
        method->attributes = read_attributes(file, method->attributes_count, cp_pools);
        methods[i] = method;
    }
    return methods;
}




void method_free(method_t **methods, u2 method_count) {
    if(methods) {
        for (int i = 0; i < method_count; i++) {
            method_t *method = methods[i];
            if(!method) continue;

            attr_free(method->attributes, method->attributes_count);
            free(method);
        }
        free(methods);
    }
}