#include "class_reader.h"
#include "../utils/bytes.h"
#include "../../../core/list/arraylist.h"
#include "constant_pool.h"
#include "field.h"
#include "method_info.h"
#include "attr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u2 slot_count_from_desciptor(char *descriptor) {
    char *ptr = descriptor + 1;
    u2 arg_count = 0;
    while(*ptr && *ptr != ')') {
        if(*ptr == '[') {
            // 数组，不影响计数
            ptr++;
            continue;
        }

        if(*ptr == 'L') {
            char *start = ptr+1;
            char *end = start;
            while(*end != ';') end++;
            ptr = end;
        }else if(*ptr == 'J' || *ptr == 'D') {
            ptr++;
        }
        ptr++;
        arg_count++;
    }
    return arg_count;
}

u2 slot_count_from_class(class_t *class) {
    u2 slot_count = 0;
    for(int i = 0; i < class->fields_count; i++) {
        slot_count += slot_count_from_desciptor(class->cp_pools[class->fields[i]->descriptor_index]);
    }
    return slot_count;
}

class_t *read_class_file(const char *path) {

    FILE *class_file;

    class_file = fopen(path, "rb");
    if(!class_file) {
        perror("class file read error");
        return NULL;
    }

    class_t *class = malloc(sizeof(class_t));
    class->magic = read_u4(class_file);
    class->minor_version = read_u2(class_file);
    class->major_version = read_u2(class_file);
    class->constant_pool_count = read_u2(class_file);
    class->cp_pools = read_constant_pool(class_file, class->constant_pool_count);
    class->access_flags = read_u2(class_file);
    class->this_class = read_u2(class_file);
    class->super_class = read_u2(class_file);
    class->interface_count = read_u2(class_file);
    // todo 暂时跳过
    read_bytes(class_file, class->interface_count);
    class->fields_count = read_u2(class_file);
    class->fields = read_fields(class_file, class->fields_count, class->cp_pools);
    class->methods_count = read_u2(class_file);
    class->methods = read_methods(class_file, class->methods_count, class->cp_pools);
    class->attributes_count = read_u2(class_file);
    class->attributes = read_attributes(class_file, class->attributes_count, class->cp_pools);

    fclose(class_file);

    void *info = class->cp_pools[class->this_class];
    check_cp_info_tag(info, CONSTANT_Class);
    cp_class_t *class_info = (cp_class_t*)info;
    class->class_name = get_utf8(class->cp_pools[class_info->name_index]);

    return class;
}






void class_free(class_t *class) {
    if(class) {
        constant_pool_free(class->cp_pools, class->constant_pool_count);
        field_free(class->fields, class->fields_count);
        method_free(class->methods, class->methods_count);
        attr_free(class->attributes, class->attributes_count);
        free(class->class_name);
        free(class);
    }
}