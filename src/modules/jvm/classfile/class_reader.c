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

class_t *read_class_file(const char *path) {

    FILE *class_file;

    class_file = fopen(path, "rb");
    if(!class_file) {
        perror("class file read error");
        return NULL;
    }

    printf("read class file: %s\n", path);

    class_t *class = calloc(1, sizeof(class_t));
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

    cp_info_t cp_info = class->cp_pools[class->this_class];
    check_cp_info_tag(cp_info.tag, CONSTANT_Class);
    class->class_name = get_utf8(&class->cp_pools[parse_to_u2(cp_info.info)]);

    if(class->fields_count > 0) {
        u2 total_field_slots = 0;
        for(u2 i=0;i<class->fields_count;i++) {
            total_field_slots += class->fields[i].slot_count;
        }
        class->total_field_slots = total_field_slots;
    }

    return class;
}






void class_free(class_t *class) {
    // todo free
}