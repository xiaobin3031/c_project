#include "../utils/bytes.h"
#include "constant_pool.h"
#include <stdio.h>
#include <stdlib.h>





int is_cp_info_tag(void *cp_info, u1 special_tag) {
    if(!cp_info) return 0;

    u1 tag = (u1)*((char *)cp_info);
    return tag == special_tag ? 1 : 0;
}

void check_cp_info_tag(void *cp_info, u1 special_tag) {
    if(!is_cp_info_tag(cp_info, special_tag)) {
        u1 tag = (u1)*((char *)cp_info);
        fprintf(stderr, "tag not match, expect %d, but got %d\n", special_tag, tag);
        exit(1);
    }
}

void **read_constant_pool(FILE *file, u2 pool_len) {
    if(pool_len <= 1) return NULL;

    void **cps = malloc(sizeof(void *) * pool_len);
    for(int i=1;i<pool_len;i++) {
        u1 tag = read_u1(file);
        switch(tag) {
            case CONSTANT_Utf8: {
                cp_utf8_t *utf8 = malloc(sizeof(cp_utf8_t));
                utf8->tag = tag;
                utf8->length = read_u2(file);
                if(utf8->length > 0) {
                    utf8->bytes = malloc(utf8->length + 1);
                    for(int j=0;j<utf8->length;j++) {
                        utf8->bytes[j] = read_u1(file);
                    }
                    utf8->bytes[utf8->length] = '\0';
                }
                cps[i] = utf8;
                break;
            }
            case CONSTANT_Integer: {
                cp_integer_t *integer = malloc(sizeof(cp_integer_t));
                integer->tag = tag;
                integer->bytes = read_u4(file);
                cps[i] = integer;
                break;
            }
            case CONSTANT_Class: {
                cp_class_t *class = malloc(sizeof(cp_class_t));
                class->tag = tag;
                class->name_index = read_u2(file);
                cps[i] = class;
                break;
            }
            case CONSTANT_NameAndType: {
                cp_nameandtype_t *nameandtype = malloc(sizeof(cp_nameandtype_t));
                nameandtype->tag = tag;
                nameandtype->name_index = read_u2(file);
                nameandtype->descriptor_index = read_u2(file);
                cps[i] = nameandtype;
                break;
            }
            case CONSTANT_Methodref: {
                cp_methodref_t *methodref = malloc(sizeof(cp_nameandtype_t));
                methodref->tag = tag;
                methodref->class_index = read_u2(file);
                methodref->name_and_type_index = read_u2(file);
                cps[i] = methodref;
                break;
            }
            case CONSTANT_Fieldref: {
                cp_fieldref_t *fieldref = malloc(sizeof(cp_fieldref_t));
                fieldref->tag = tag;
                fieldref->class_index = read_u2(file);
                fieldref->name_and_type_index = read_u2(file);
                cps[i] = fieldref;
                break;
            }
            // todo 后续扩展更多tag
            default: {
                printf("unknown tag type: %d\n", tag);
                exit(1);
            }
        }
    }
    return cps;
}

char *get_utf8(void *cp_info) {
    check_cp_info_tag(cp_info, CONSTANT_Utf8);
    cp_utf8_t *utf8 = (cp_utf8_t *)cp_info;
    return utf8->bytes;
}

void print_class_name(void **cp_pools, u2 pool_size) {
    for(int i=0;i<pool_size;i++) {
        void *info = cp_pools[i];
        if(is_cp_info_tag(info, CONSTANT_Class)) {
            cp_class_t *class = (cp_class_t *)info;
            char *clsName = get_utf8(cp_pools[class->name_index]);
            printf("class name: %s\n", clsName);
        }
    }
}

void print_method_name(void **cp_pools, u2 pool_size) {
    for(int i=0;i<pool_size;i++) {
        void *info = cp_pools[i];
        if(is_cp_info_tag(info, CONSTANT_Methodref)) {
            cp_methodref_t *methodref = (cp_methodref_t *)info;
            info = cp_pools[methodref->name_and_type_index];
            check_cp_info_tag(info, CONSTANT_NameAndType);
            cp_nameandtype_t *name = (cp_nameandtype_t *)info;
            char *methodName = get_utf8(cp_pools[name->name_index]);
            printf("method name: %s\n", methodName);
        }
    }
}


void constant_pool_free(void **cp_pools, u2 pool_size) {
    if(cp_pools) {
        for(int i=0;i<pool_size;i++) {
            if(cp_pools[i]) free(cp_pools[i]);
        }
        free(cp_pools);
    }
}