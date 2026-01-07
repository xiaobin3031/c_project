#include "field.h"
#include "../utils/bytes.h"
#include "class_bytes.h"
#include "attr.h"
#include "../utils/slots.h"
#include <stdlib.h>

field_file_t *read_fields(FILE *file, u2 field_count, cp_info_t *cp_pools) {
    if(field_count == 0) return NULL;

    field_file_t *fields = malloc(field_count * sizeof(field_file_t));
    u2 offset = 0;
    for(int i = 0; i < field_count; i++) { 
        field_file_t field;
        field.access_flags = read_u2(file);
        u2 name_index = read_u2(file);
        field.name = get_utf8(&cp_pools[name_index]);
        u2 descriptor_index = read_u2(file);
        field.descriptor = get_utf8(&cp_pools[descriptor_index]);
        field.attributes_count = read_u2(file);
        field.attributes = read_attributes(file, field.attributes_count, cp_pools);

        field.slot_offset_in_class = offset++;
        field.slot_count = slot_count_from_desciptor(field.descriptor);
        offset += field.slot_count;
        fields[i] = field;
    }

    return fields;
}

field_file_t *read_fields_bytes(class_file_bytes_t *class_bytes, u2 field_count, cp_info_t *cp_pools) {
    if(field_count == 0) return NULL;

    field_file_t *fields = malloc(field_count * sizeof(field_file_t));
    u2 offset = 0;
    for(int i = 0; i < field_count; i++) { 
        field_file_t field;
        field.access_flags = read_bytes_u2(class_bytes);
        u2 name_index = read_bytes_u2(class_bytes);
        field.name = get_utf8(&cp_pools[name_index]);
        u2 descriptor_index = read_bytes_u2(class_bytes);
        field.descriptor = get_utf8(&cp_pools[descriptor_index]);
        field.attributes_count = read_bytes_u2(class_bytes);
        field.attributes = read_attributes_bytes(class_bytes, field.attributes_count, cp_pools);

        field.slot_offset_in_class = offset++;
        field.slot_count = slot_count_from_desciptor(field.descriptor);
        offset += field.slot_count;
        fields[i] = field;
    }

    return fields;
}


void field_free(field_file_t *fields, u2 field_count) {
    // todo free
}