#include "class_reader.h"
#include "../utils/bytes.h"
#include "constant_pool.h"
#include "field.h"
#include "method_info.h"
#include "attr.h"
#include <stdio.h>
#include <stdlib.h>

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

    return class;
}






void class_free(class_t *class) {
    if(class) {
        constant_pool_free(class->cp_pools, class->constant_pool_count);
        field_free(class->fields, class->fields_count);
        method_free(class->methods, class->methods_count);
        attr_free(class->attributes, class->attributes_count);
        free(class);
    }
}