#include "method_info.h"
#include "class_reader.h"
#include "attr.h"
#include <stdlib.h>

int method_is_flag(method_t *method, method_acc_flags flag) {
    return (method->access_flags & flag) == flag ? 1 : 0;
}

method_t **read_methods(FILE *file, u2 method_count, void **cp_pools) {
    method_t **methods = malloc(sizeof(method_t *) * method_count);
    for (int i = 0; i < method_count; i++) { 
        method_t *method = malloc(sizeof(method_t));
        method->access_flags = read_u2(file);
        method->name_index = read_u2(file);
        method->descriptor_index = read_u2(file);
        method->attributes_count = read_u2(file);
        method->attributes = read_attributes(file, method->attributes_count, cp_pools);

        // 解析param信息
        void *info = cp_pools[method->descriptor_index];
        char *descriptor = get_utf8(info);
        u2 arg_count = slot_count_from_desciptor(descriptor);
        if(arg_count > 0) {
            // todo 暂时不解析类型，还没想好后续怎么使用
        }

        method->arg_slot_count = arg_count;

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