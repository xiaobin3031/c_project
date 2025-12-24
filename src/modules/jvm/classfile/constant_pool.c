#include "../utils/bytes.h"
#include "constant_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




int is_cp_info_tag(u1 tag, u1 special_tag) {
    return tag == special_tag ? 1 : 0;
}

void check_cp_info_tag(u1 tag, u1 special_tag) {
    if(tag != special_tag) {
        fprintf(stderr, "tag not match, expect %d, but got %d\n", special_tag, tag);
        abort();
    }
}

cp_info_t *read_constant_pool(FILE *file, u2 pool_len) {
    if(pool_len <= 1) return NULL;

    cp_info_t *cps = malloc(sizeof(cp_info_t) * pool_len);
    for(int i=1;i<pool_len;i++) {
        u1 tag = read_u1(file);
        cp_info_t info;
        info.tag = tag;
        switch(tag) {
            case CONSTANT_Utf8: {
                u2 high = read_u1(file);
                u2 low = read_u1(file);
                u2 length = (high << 8) | low;
                u1 *utf8_bytes = malloc(2 + length + 1);
                if(length > 0) {
                    u1 *bytes = read_bytes(file, length);
                    memcpy(utf8_bytes + 2, bytes, length);
                    free(bytes);
                }
                utf8_bytes[0] = high;
                utf8_bytes[1] = low;
                utf8_bytes[2 + length] = '\0';
                info.info = utf8_bytes;
                break;
            }
            case CONSTANT_Integer: 
            case CONSTANT_NameAndType:
            case CONSTANT_Methodref:
            case CONSTANT_Fieldref:
            case CONSTANT_InterfaceMethodref:
            case CONSTANT_Float: {
                info.info = read_bytes(file, 4);
                break;
            }
            case CONSTANT_Class: {
                info.info = read_bytes(file, 2);
                break;
            }
            // todo 后续扩展更多tag
            default: {
                printf("unknown tag type: %d\n", tag);
                abort();
            }
        }
        cps[i] = info;
    }
    return cps;
}

char *get_utf8(cp_info_t *cp_info) {
    check_cp_info_tag(cp_info->tag, CONSTANT_Utf8);
    return cp_info->info + 2;
}


void constant_pool_free(void **cp_pools, u2 pool_size) {
    // todo free
}