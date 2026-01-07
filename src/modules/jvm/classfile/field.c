#include "field.h"
#include "../utils/bytes.h"
#include "class_bytes.h"
#include "attr.h"
#include <stdlib.h>

field_file_t *read_fields(FILE *file, u2 field_count, cp_info_t *cp_pools) {
    if(field_count == 0) return NULL;

    field_file_t *fields = malloc(field_count * sizeof(field_file_t));
    u2 offset = 0;
    for(int i = 0; i < field_count; i++) { 
        field_file_t *field = &fields[i];
        field->access_flags = read_u2(file);
        field->name_index = read_u2(file);
        field->descriptor_index = read_u2(file);
        field->attributes_count = read_u2(file);
        field->attributes = read_attributes(file, field->attributes_count, cp_pools);
    }

    return fields;
}

field_file_t *read_fields_bytes(class_file_bytes_t *class_bytes, u2 field_count, cp_info_t *cp_pools) {
    if(field_count == 0) return NULL;

    field_file_t *fields = malloc(field_count * sizeof(field_file_t));
    u2 offset = 0;
    for(int i = 0; i < field_count; i++) { 
        field_file_t *field = &fields[i];
        field->access_flags = read_bytes_u2(class_bytes);
        field->name_index = read_bytes_u2(class_bytes);
        field->descriptor_index = read_bytes_u2(class_bytes);
        field->attributes_count = read_bytes_u2(class_bytes);
        field->attributes = read_attributes_bytes(class_bytes, field->attributes_count, cp_pools);
    }

    return fields;
}


void field_free(field_file_t *fields, u2 field_count) {
    // todo free
}