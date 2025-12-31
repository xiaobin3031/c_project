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
        if(attr.attribute_length > 0 
            && strcmp(attr_name, "Code") != 0
            && strcmp(attr_name, "Exceptions") != 0
            ) {
            attr.info = read_bytes(file, attr.attribute_length);
        }
        if(strcmp(attr_name, "Code") == 0) {
            attr.tag = ATTR_CODE;
            attr_code_t *code_attr = malloc(sizeof(attr_code_t));
            code_attr->max_stack = read_u2(file);
            code_attr->max_locals = read_u2(file);
            code_attr->code_length = read_u4(file);
            code_attr->code = read_bytes(file, code_attr->code_length);
            code_attr->exception_table_length = read_u2(file);
            if(code_attr->exception_table_length > 0) {
                code_attr->exception_table = malloc(code_attr->exception_table_length * sizeof(exception_table_t));
                for(u2 j = 0; j < code_attr->exception_table_length; j++) {
                    code_attr->exception_table[j].start_pc = read_u2(file);
                    code_attr->exception_table[j].end_pc = read_u2(file);
                    code_attr->exception_table[j].handler_pc = read_u2(file);
                    code_attr->exception_table[j].catch_type = read_u2(file);
                }
            } else {
                code_attr->exception_table = NULL;
            }
            code_attr->attributes_count = read_u2(file);
            code_attr->attributes = read_attributes(file, code_attr->attributes_count, cp_pools);
            attr.info = (u1 *)code_attr;
        } else if(strcmp(attr_name, "StackMapTable") == 0) {
            attr.tag = ATTR_STACK_MAP_TABLE;
        } else if(strcmp(attr_name, "Signature") == 0) {
            attr.tag = ATTR_SIGNATURE;
        } else if(strcmp(attr_name, "RuntimeVisibleAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_VISIBLE_ANNOTATIONS;
        } else if(strcmp(attr_name, "RuntimeInvisibleAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_INVISIBLE_ANNOTATIONS;
        } else if(strcmp(attr_name, "RuntimeVisibleParameterAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS;
        } else if(strcmp(attr_name, "RuntimeInvisibleParameterAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS;
        } else if(strcmp(attr_name, "RuntimeVisibleTypeAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_VISIBLE_TYPE_ANNOTATIONS;
        }else if(strcmp(attr_name, "RuntimeInvisibleTypeAnnotations") == 0) {
            attr.tag = ATTR_RUNTIME_INVISIBLE_TYPE_ANNOTATIONS;
        } else if(strcmp(attr_name, "AnnotationDefault") == 0) {
            attr.tag = ATTR_ANNOTATION_DEFAULT;
        } else if(strcmp(attr_name, "BootstrapMethods") == 0) {
            attr.tag = ATTR_BOOTSTRAP_TABLE;
        } else if(strcmp(attr_name, "Module") == 0) {
            attr.tag = ATTR_MODULE;
        } else if(strcmp(attr_name, "ModulePackages") == 0) {
            attr.tag = ATTR_MODULE_PACKAGE;
        } else if(strcmp(attr_name, "PermittedSubclasses") == 0) {
            attr.tag = ATTR_PERMITTED_SUBCLASSES;
        }else if(strcmp(attr_name, "Record") == 0) {
            attr.tag = ATTR_RECORD;
        }else if(strcmp(attr_name, "ConstantValue") == 0) {
            attr.tag = ATTR_CONSTANT_VALUE;
        }else if(strcmp(attr_name, "Exceptions") == 0) {
            attr.tag = ATTR_EXCEPTIONS;
            
        }else if(strcmp(attr_name, "InnerClasses") == 0) {
            attr.tag = ATTR_INNER_CLASSES;
        }else if(strcmp(attr_name, "LineNumberTable") == 0) {
            attr.tag = ATTR_LINE_NUMBER_TABLE;
        }else if(strcmp(attr_name, "LocalVariableTable") == 0) {
            attr.tag = ATTR_LOCAL_VARIABLE_TABLE;
        }else if(strcmp(attr_name, "LocalVariableTypeTable") == 0) {
            attr.tag = ATTR_LOCAL_VARIABLE_TYPE_TABLE;
        }else if(strcmp(attr_name, "MethodParameters") == 0) {
            attr.tag = ATTR_METHOD_PARAMETERS;
        }else if(strcmp(attr_name, "EnclosingMethod") == 0) {
            attr.tag = ATTR_ENCLOSING_METHOD;
        } else if(strcmp(attr_name, "Synthetic") == 0) {
            attr.tag = ATTR_SYNTHETIC;
        }else if(strcmp(attr_name, "SourceFile") == 0) {
            attr.tag = ATTR_SOURCE_FILE;
        }else if(strcmp(attr_name, "SourceDebugExtension") == 0) {
            attr.tag = ATTR_SOURCE_DEBUG_EXTENSION;
        } else if(strcmp(attr_name, "Deprecated") == 0) {
            attr.tag = ATTR_DEPRECATED;
        }else if(strcmp(attr_name, "NestHost") == 0) {
            attr.tag = ATTR_NEST_HOST;
        }else if(strcmp(attr_name, "NestMembers") == 0) {
            attr.tag = ATTR_NEST_MEMBERS;
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