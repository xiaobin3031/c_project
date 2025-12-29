#include "../utils/bytes.h"
#include "constant_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


cp_methodref_t *get_methodref(cp_info_t *cp_info) { 
    check_cp_info_tag(cp_info->tag, CONSTANT_Methodref);
    return (cp_methodref_t *)cp_info->info;
}

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
            case CONSTANT_MethodHandle: {
                info.info = read_bytes(file, 3);
                break;
            }
            case CONSTANT_Methodref: {
                cp_methodref_t *methodref = malloc(sizeof(cp_methodref_t));
                methodref->class_index = read_u2(file);
                methodref->name_and_type_index = read_u2(file);
                methodref->resolved_method = NULL;
                info.info = (u1*) methodref;
                break;
            }
            case CONSTANT_Fieldref: {
                cp_fieldref_t *fieldref = malloc(sizeof(cp_fieldref_t));
                fieldref->class_index = read_u2(file);
                fieldref->name_and_type_index = read_u2(file);
                fieldref->resolved_field = NULL;
                info.info = (u1*) fieldref;
                break;
            }
            case CONSTANT_NameAndType: {
                cp_nameandtype_t *nameandtype = malloc(sizeof(cp_nameandtype_t));
                nameandtype->name_index = read_u2(file);
                nameandtype->descriptor_index = read_u2(file);
                info.info = (u1*) nameandtype;
                break;
            }
            case CONSTANT_Integer: 
            case CONSTANT_InterfaceMethodref:
            case CONSTANT_Dynamic:
            case CONSTANT_InvokeDynamic:
            case CONSTANT_Float: {
                info.info = read_bytes(file, 4);
                break;
            }
            case CONSTANT_Long:
            case CONSTANT_Double: {
                info.info = read_bytes(file, 8);
                cps[i] = info;
                i++;

                cps[i].tag = 0;
                cps[i].info = NULL;
                break;
            }
            case CONSTANT_Class: {
                cp_class_t *class = malloc(sizeof(cp_class_t));
                class->name_index = read_u2(file);
                info.info = (u1*) class;
                break;
            }
            case CONSTANT_MethodType:
            case CONSTANT_String:
             {
                info.info = read_bytes(file, 2);
                break;
            }
            // todo 后续扩展更多tag
            default: {
                printf("constant pool unknown tag type: %d\n", tag);
                abort();
            }
        }
        if(tag != CONSTANT_Long && tag != CONSTANT_Double) {
            cps[i] = info;
        }
    }
    return cps;
}

char *get_utf8(cp_info_t *cp_info) {
    check_cp_info_tag(cp_info->tag, CONSTANT_Utf8);
    return (char*)(cp_info->info + 2);
}


void constant_pool_free(void **cp_pools, u2 pool_size) {
    // todo free
}