#include "method_info.h"
#include "class_reader.h"
#include "../utils/slots.h"
#include "attr.h"
#include <stdlib.h>

int method_is_flag(u2 access_flag, method_acc_flags flag) {
    return (access_flag & flag) == flag ? 1 : 0;
}

method_t *read_methods(FILE *file, u2 method_count, cp_info_t *cp_pools) {
    method_t *methods = malloc(sizeof(method_t) * method_count);
    for (int i = 0; i < method_count; i++) { 
        method_t method;
        method.access_flags = read_u2(file);
        method.name_index = read_u2(file);
        method.descriptor_index = read_u2(file);
        method.attributes_count = read_u2(file);
        method.attributes = read_attributes(file, method.attributes_count, cp_pools);

        // 解析param信息
        void *info = &cp_pools[method.descriptor_index];
        char *descriptor = get_utf8(info);
        u2 arg_count = slot_count_from_desciptor(descriptor);
        if(arg_count > 0) {
            // todo 暂时不解析类型，还没想好后续怎么使用
        }

        method.arg_slot_count = arg_count;

        methods[i] = method;
    }
    return methods;
}




void method_free(method_t *methods, u2 method_count) {
    // todo free
}