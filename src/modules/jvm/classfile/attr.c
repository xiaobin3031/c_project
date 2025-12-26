#include "attr.h"
#include "constant_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int is_attr_tag(u1 tag, u1 special_tag) {
    return tag == special_tag;
}

attribute_t *read_attributes(FILE *file, u2 attr_count, cp_info_t *cp_pools) {
    if(attr_count <= 0) return NULL;

    attribute_t *attrs = malloc(attr_count * sizeof(attribute_t));
    for(u2 i = 0; i < attr_count; i++) {
        u2 attr_name_index = read_u2(file);
        cp_info_t info = cp_pools[attr_name_index];
        char *attr_name = get_utf8(&info);
        attribute_t attr;
        attr.attribute_name_index = attr_name_index;
        attr.attribute_length = read_u4(file);
        if(attr.attribute_length > 0) {
            attr.info = read_bytes(file, attr.attribute_length);
        }
        if(strcmp(attr_name, "Code") == 0) {
            attr.tag = ATTR_CODE;
        } else if(strcmp(attr_name, "StackMapTable") == 0) {
            attr.tag = ATTR_STACK_MAP_TABLE;
        }
        else {
            printf("unknown attribute [%s]\n", attr_name);
            // abort();
        }
        attrs[i] = attr;
    }
    return attrs;
}




void attr_free(attribute_t *attrs, u2 attr_count) {
    // todo free
}