#include "class_reader.h"
#include "../utils/bytes.h"
#include "constant_pool.h"
#include "field.h"
#include "method_info.h"
#include "attr.h"
#include "class_bytes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class_file_t *read_class_file(const char *path) {

    FILE *class_file;

    class_file = fopen(path, "rb");
    if(!class_file) {
        return NULL;
    }

    // printf("read class file: %s\n", path);

    class_file_t *class = calloc(1, sizeof(class_file_t));
    class->magic = read_u4(class_file);
    class->minor_version = read_u2(class_file);
    class->major_version = read_u2(class_file);
    class->constant_pool_count = read_u2(class_file);
    class->cp_pools = read_constant_pool(class_file, class->constant_pool_count);
    class->access_flags = read_u2(class_file);
    class->this_class = read_u2(class_file);
    class->super_class = read_u2(class_file);
    class->interface_count = read_u2(class_file);
    if(class->interface_count > 0) {
        class->interfaces = calloc(class->interface_count, sizeof(u2));
        for(u2 i=0;i<class->interface_count;i++) {
            class->interfaces[i] = read_u2(class_file);
        }
    }
    class->fields_count = read_u2(class_file);
    class->fields = read_fields(class_file, class->fields_count, class->cp_pools);
    class->methods_count = read_u2(class_file);
    class->methods = read_methods(class_file, class->methods_count, class->cp_pools);
    class->attributes_count = read_u2(class_file);
    class->attributes = read_attributes(class_file, class->attributes_count, class->cp_pools);

    // 判断是否是读取到最后了
    long cur = ftell(class_file);
    fseek(class_file, 0, SEEK_END);
    long end = ftell(class_file);
    if (cur != end) {
        fprintf(stderr, "class file %s is not end\n", path);
        fclose(class_file);
        return NULL;
    }

    fclose(class_file);

    return class;
}

class_file_t *read_by_class_bytes(class_file_bytes_t *class_bytes) {

    class_file_t *class = calloc(1, sizeof(class_file_t));
    class->magic = read_bytes_u4(class_bytes);
    class->minor_version = read_bytes_u2(class_bytes);
    class->major_version = read_bytes_u2(class_bytes);
    class->constant_pool_count = read_bytes_u2(class_bytes);
    class->cp_pools = read_constant_pool_bytes(class_bytes, class->constant_pool_count);
    class->access_flags = read_bytes_u2(class_bytes);
    class->this_class = read_bytes_u2(class_bytes);
    class->super_class = read_bytes_u2(class_bytes);
    class->interface_count = read_bytes_u2(class_bytes);
    if(class->interface_count > 0) {
        class->interfaces = calloc(class->interface_count, sizeof(u2));
        for(u2 i=0;i<class->interface_count;i++) {
            class->interfaces[i] = read_bytes_u2(class_bytes);
        }
    }
    class->fields_count = read_bytes_u2(class_bytes);
    class->fields = read_fields_bytes(class_bytes, class->fields_count, class->cp_pools);
    class->methods_count = read_bytes_u2(class_bytes);
    class->methods = read_methods_bytes(class_bytes, class->methods_count, class->cp_pools);
    class->attributes_count = read_bytes_u2(class_bytes);
    class->attributes = read_attributes_bytes(class_bytes, class->attributes_count, class->cp_pools);

    // 判断是否是读取到最后了
    if (class_bytes->offset != class_bytes->size) {
        printf("class file bytes is not end\n");
        return NULL;
    }

    return class;
}


int is_class(class_file_t *class) {
    return !(
        class->access_flags & CLASS_ACC_INTERFACE
        || class->access_flags & CLASS_ACC_ANNOTATION
        || class->access_flags & CLASS_ACC_MODULE
        || class->access_flags & CLASS_ACC_ENUM
        || class->access_flags & CLASS_ACC_SYNTHETIC
    );
}





void class_free(class_file_t *class) {
    // todo free
}