#include "class.h"
#include "../utils/bytes.h"
#include "../utils/slots.h"
#include "../classfile/class_reader.h"
#include "../classfile/constant_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>


class_t *define_class(class_file_t *class_file) {
    if(class_file == NULL) return NULL;

    class_t *klass = calloc(1, sizeof(class_t));
    klass->access_flags = class_file->access_flags;
    cp_info_t cp_info = class_file->cp_pools[class_file->this_class];
    check_cp_info_tag(cp_info.tag, CONSTANT_Class);
    char *class_name = strdup(get_utf8(&class_file->cp_pools[((cp_class_t*)cp_info.info)->name_index]));
    klass->class_name = strdup(class_name);
    char *ptr = class_name;
    char *simple_name = ptr;
    while(*ptr != '\0') {
        if(*ptr == '/') {
            simple_name = ptr + 1;
        }else if(*ptr == '.'){
            *ptr = '\0';
            break;
        }
        ptr++;
    }
    klass->class_simple_name = strdup(simple_name);
    free(class_name);


    cp_info_t *cp_pools = class_file->cp_pools;

    // fields
    if(class_file->fields_count > 0) {
        u2 total_field_slots = 0;
        klass->fields = calloc(class_file->fields_count, sizeof(field_t));
        for(u2 i=0;i<class_file->fields_count;i++) {
            field_file_t *field_file = &class_file->fields[i];
            field_t *field = &klass->fields[i];
            field->access_flags = field_file->access_flags;
            field->slot_id = i;
            char *name = get_utf8(&cp_pools[field_file->name_index]);
            char *descriptor = get_utf8(&cp_pools[field_file->descriptor_index]);
            field->slot_count = slot_count_from_desciptor(descriptor);
            field->name = strdup(name);
            field->descriptor = strdup(descriptor);
            field->slots = calloc(field->slot_count, sizeof(slot_t));

            total_field_slots += field->slot_count;
        }
        klass->total_field_slots = total_field_slots;
    }

    // methods
    if(class_file->methods_count > 0) {
        klass->methods = calloc(class_file->methods_count, sizeof(method_t));
        for(u2 i=0;i<class_file->methods_count;i++) {
            method_file_t *method_file = &class_file->methods[i];
            method_t *method = &klass->methods[i];
            method->access_flags = method_file->access_flags;
            char *name = get_utf8(&cp_pools[method_file->name_index]);
            char *descriptor = get_utf8(&cp_pools[method_file->descriptor_index]);
            method->name = strdup(name);
            method->arg_slot_count = slot_count_from_desciptor(descriptor);
            char *ptr = descriptor;
            while(*ptr != ')') ptr++;
            method->return_slot_count = slot_count_from_desciptor(ptr + 1);

            for(u2 j = 0;j<method_file->attributes_count;j++) {
                attribute_file_t *attr = &method_file->attributes[j];
                if(attr->tag == ATTR_CODE) {
                    attr_file_code_t *attr_code = (attr_file_code_t*)attr->info;
                    method->code = calloc(1, sizeof(rt_code_t));
                    method->code->codes = attr_code->code;
                    method->code->code_length = attr_code->code_length;
                    method->code->max_locals = attr_code->max_locals;
                    method->code->max_stack = attr_code->max_stack;
                    method->exception_table_length = attr_code->exception_table_length;
                    method->exception_table = (rt_exception_table_t *)attr_code->exception_table;
                    break;
                }
            }
        }
    }
    
    // 复制constant_pool
    if(class_file->constant_pool_count > 0) {
        klass->rt_cp_count = class_file->constant_pool_count;
        klass->entries = calloc(class_file->constant_pool_count, sizeof(rt_cp_entry_t));
        for(u2 i=1;i<class_file->constant_pool_count;i++) {
            rt_cp_entry_t *entry = &klass->entries[i];
            cp_info_t *cp_info = &cp_pools[i];
            entry->tag = cp_info->tag;
            entry->resolved = 0;
            switch(cp_info->tag) {
                case CONSTANT_Class: {
                    cp_class_t *cp_class = (cp_class_t*)cp_info->info;
                    entry->sym.class_name = get_utf8(&cp_pools[cp_class->name_index]);
                    break;
                }
                case CONSTANT_Methodref: {
                    cp_methodref_t *cp_methodref = (cp_methodref_t*)cp_info->info;
                    cp_class_t *cp_class = get_cp_class(&cp_pools[cp_methodref->class_index]);
                    entry->sym.class_name = get_utf8(&cp_pools[cp_class->name_index]);
                    cp_nameandtype_t *cp_nameandtype = get_cp_nameandtype(&cp_pools[cp_methodref->name_and_type_index]);
                    entry->sym.name = get_utf8(&cp_pools[cp_nameandtype->name_index]);
                    entry->sym.descriptor = get_utf8(&cp_pools[cp_nameandtype->descriptor_index]);
                    break;
                }
                case CONSTANT_Fieldref: {
                    cp_fieldref_t *cp_fieldref = get_cp_fieldref(cp_info);
                    cp_nameandtype_t *cp_nameandtype = get_cp_nameandtype(&cp_pools[cp_fieldref->name_and_type_index]);
                    entry->sym.name = get_utf8(&cp_pools[cp_nameandtype->name_index]);
                    entry->sym.descriptor = get_utf8(&cp_pools[cp_nameandtype->descriptor_index]);
                    break;
                }
                case CONSTANT_String: {
                    cp_string_t *cp_string = (cp_string_t*)cp_info->info;
                    char *string = get_utf8(&cp_pools[cp_string->string_index]);
                    entry->strings = string_new(string);
                    break;
                }
                case CONSTANT_Integer: {
                    cp_integer_t *cp_integer = (cp_integer_t*)cp_info->info;
                    entry->number.int_value = (int32_t)cp_integer->bytes;
                    break;
                }
                case CONSTANT_Long: {
                    uint64_t high = parse_to_u4(cp_info->info);
                    u4 low = parse_to_u4(cp_info->info + 4);
                    entry->number.long_value = (int64_t)(high << 32 | low);
                    // 跳过另外一个constant
                    i++;
                    break;
                }
                case CONSTANT_Float: {
                    float f;
                    u4 value = parse_to_u4(cp_info->info);
                    memcpy(&f, &value, sizeof(float));
                    entry->number.float_value = f;
                    break;
                }
                case CONSTANT_Double: {
                    double d;
                    uint64_t high = parse_to_u4(cp_info->info);
                    u4 low = parse_to_u4(cp_info->info + 4);
                    int64_t value = (int64_t)(high << 32 | low);
                    memcpy(&d, &value, sizeof(double));
                    entry->number.double_value = d;

                    // 跳过另外一个constant
                    i++;
                    break;
                }
                default: { 
                    printf("skip convert constant pool to runtime constant pool, tag: %d\n", cp_info->tag);
                    break;
                }
            }
        }
    }

    // super
    if(class_file->super_class > 0) {
        cp_class_t *super_class = get_cp_class(&cp_pools[class_file->super_class]);
        char *super_class_name = get_utf8(&cp_pools[super_class->name_index]);
        klass->super_class_name = strdup(super_class_name);
    }

    klass->interface_count = class_file->interface_count;
    if(class_file->interface_count > 0) {
        klass->interface_class_names = calloc(class_file->interface_count, sizeof(char *));
        for(u2 i = 0; i < class_file->interface_count; i++) {
            cp_class_t *interface_class = get_cp_class(&cp_pools[class_file->interfaces[i]]);
            char *interface_class_name = get_utf8(&cp_pools[interface_class->name_index]);
            klass->interface_class_names[i] = strdup(interface_class_name);
        }
    }

    klass->state = CLASS_LOADED;
    pthread_mutex_init(&klass->lock, NULL);

    return klass;
}

native_string_t *string_new(const char *utf8) {
    native_string_t *native_string = calloc(1, sizeof(native_string_t));
    native_string->utf8 = strdup(utf8);
    native_string->length = strlen(utf8);
    return native_string;
}