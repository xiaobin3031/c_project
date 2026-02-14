#include "class.h"
#include "../utils/bytes.h"
#include "../utils/slots.h"
#include "../classfile/class_reader.h"
#include "../classfile/constant_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static void clean_entry(rt_cp_entry_t *entry) {
    entry->tag = 0;
    entry->klass = NULL;
    entry->method = NULL;
    entry->field = NULL;
    entry->sym.class_name = NULL;
    entry->sym.descriptor = NULL;
    entry->sym.name = NULL;
    entry->number.double_value = 0;
    entry->number.float_value = 0;
    entry->number.long_value = 0;
    entry->number.int_value = 0;
    entry->resolved = 0;
}
char *resolve_class_name(class_file_t *cf) {
    cp_info_t *cp_info = &cf->cp_pools[cf->this_class];
    cp_class_t *cp_class = get_cp_class(cp_info);
    return get_utf8_copy(&cf->cp_pools[cp_class->name_index]);
}

class_t *define_class(class_file_t *class_file) {
    if(class_file == NULL) return NULL;

    class_t *klass = calloc(1, sizeof(class_t));
    klass->access_flags = class_file->access_flags;
    char *class_name = resolve_class_name(class_file);
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
        klass->fields_count = class_file->fields_count;
        klass->fields = calloc(class_file->fields_count, sizeof(field_t));
        for(u2 i=0;i<class_file->fields_count;i++) {
            field_file_t *field_file = &class_file->fields[i];
            field_t *field = &klass->fields[i];
            field->access_flags = field_file->access_flags;
            field->slot_id = i;
            field->name = get_utf8_copy(&cp_pools[field_file->name_index]);
            field->descriptor = get_utf8_copy(&cp_pools[field_file->descriptor_index]);
            field->slot_count = slot_count_from_desciptor(field->descriptor);
            field->slots = calloc(field->slot_count, sizeof(slot_t));

            total_field_slots += field->slot_count;
        }
        klass->total_field_slots = total_field_slots;
    }

    if(klass->access_flags & CLASS_ACC_ENUM) {
        // 添加两个字段， String name 和 int ordinal
        klass->total_field_slots += 2;
    }

    // methods
    if(class_file->methods_count > 0) {
        klass->methods_count = class_file->methods_count;
        klass->methods = calloc(class_file->methods_count, sizeof(method_t));
        for(u2 i=0;i<class_file->methods_count;i++) {
            method_file_t *method_file = &class_file->methods[i];
            method_t *method = &klass->methods[i];
            method->klass = klass;
            method->access_flags = method_file->access_flags;
            method->name = get_utf8_copy(&cp_pools[method_file->name_index]);
            method->descriptor = get_utf8_copy(&cp_pools[method_file->descriptor_index]);
            method->arg_slot_count = slot_count_from_desciptor(method->descriptor);
            char *ptr = method->descriptor;
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
                    method->exception_table = calloc(attr_code->exception_table_length, sizeof(rt_exception_table_t));
                    for(u2 k = 0; k < attr_code->exception_table_length; k++) {
                        rt_exception_table_t *exception_table = &method->exception_table[k];
                        attr_file_exception_table_t *attr_exception_table = &attr_code->exception_table[k];
                        exception_table->catch_type = attr_exception_table->catch_type;
                        exception_table->end_pc = attr_exception_table->end_pc;
                        exception_table->handler_pc = attr_exception_table->handler_pc;
                        exception_table->start_pc = attr_exception_table->start_pc;
                    }
                    break;
                }else if(attr->tag == ATTR_EXCEPTIONS) {
                    u1 *info = attr->info;
                    method->exception_count = parse_to_u2(info);
                    u2 offset = 2;
                    if(method->exception_count > 0) {
                        method->exceptions = calloc(method->exception_count, sizeof(rt_exception_t));
                        for(u2 i = 0; i < method->exception_count; i++) {
                            u2 index = parse_to_u2(info + offset);
                            cp_class_t *cp_class = get_cp_class(class_file->cp_pools + index);
                            method->exceptions[i].class_name = get_utf8_copy(class_file->cp_pools + cp_class->name_index);
                            offset += 2;
                        }
                    }
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
            clean_entry(entry);
            cp_info_t *cp_info = &cp_pools[i];
            entry->tag = cp_info->tag;
            switch(cp_info->tag) {
                case CONSTANT_Class: {
                    cp_class_t *cp_class = (cp_class_t*)cp_info->info;
                    entry->sym.class_name = get_utf8_copy(&cp_pools[cp_class->name_index]);
                    break;
                }
                case CONSTANT_Methodref: {
                    cp_methodref_t *cp_methodref = (cp_methodref_t*)cp_info->info;
                    cp_class_t *cp_class = get_cp_class(&cp_pools[cp_methodref->class_index]);
                    entry->sym.class_name = get_utf8_copy(&cp_pools[cp_class->name_index]);
                    cp_nameandtype_t *cp_nameandtype = get_cp_nameandtype(&cp_pools[cp_methodref->name_and_type_index]);
                    entry->sym.name = get_utf8_copy(&cp_pools[cp_nameandtype->name_index]);
                    entry->sym.descriptor = get_utf8_copy(&cp_pools[cp_nameandtype->descriptor_index]);
                    break;
                }
                case CONSTANT_Fieldref: {
                    cp_fieldref_t *cp_fieldref = get_cp_fieldref(cp_info);
                    cp_class_t *cp_class = get_cp_class(&cp_pools[cp_fieldref->class_index]);
                    entry->sym.class_name = get_utf8_copy(&cp_pools[cp_class->name_index]);
                    cp_nameandtype_t *cp_nameandtype = get_cp_nameandtype(&cp_pools[cp_fieldref->name_and_type_index]);
                    entry->sym.name = get_utf8_copy(&cp_pools[cp_nameandtype->name_index]);
                    entry->sym.descriptor = get_utf8_copy(&cp_pools[cp_nameandtype->descriptor_index]);
                    break;
                }
                case CONSTANT_InterfaceMethodref: {
                    cp_interfacemethodref_t *cp_imr = (cp_interfacemethodref_t*)cp_info->info;
                    cp_class_t *cp_class = get_cp_class(&cp_pools[cp_imr->class_index]);
                    entry->sym.class_name = get_utf8_copy(&cp_pools[cp_class->name_index]);
                    cp_nameandtype_t *cp_nameandtype = get_cp_nameandtype(&cp_pools[cp_imr->name_and_type_index]);
                    entry->sym.name = get_utf8_copy(&cp_pools[cp_nameandtype->name_index]);
                    entry->sym.descriptor = get_utf8_copy(&cp_pools[cp_nameandtype->descriptor_index]);
                    break;
                }
                case CONSTANT_String: {
                    cp_string_t *cp_string = (cp_string_t*)cp_info->info;
                    char *string = get_utf8_copy(&cp_pools[cp_string->string_index]);
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
                    // printf("skip convert constant pool to runtime constant pool, tag: %d\n", cp_info->tag);
                    break;
                }
            }
        }
    }

    // super
    if(class_file->super_class > 0) {
        cp_class_t *super_class = get_cp_class(&cp_pools[class_file->super_class]);
        klass->super_class_name = get_utf8_copy(&cp_pools[super_class->name_index]);
    }

    klass->interface_count = class_file->interface_count;
    if(class_file->interface_count > 0) {
        klass->interface_class_names = calloc(class_file->interface_count, sizeof(char *));
        for(u2 i = 0; i < class_file->interface_count; i++) {
            cp_class_t *interface_class = get_cp_class(&cp_pools[class_file->interfaces[i]]);
            klass->interface_class_names[i] = get_utf8_copy(&cp_pools[interface_class->name_index]);
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