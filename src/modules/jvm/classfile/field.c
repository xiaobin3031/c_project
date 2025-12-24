#include "field.h"
#include "../utils/bytes.h"
#include "attr.h"
#include "../utils/slots.h"
#include <stdlib.h>

field_t *read_fields(FILE *file, u2 field_count, cp_info_t *cp_pools) {
    if(field_count == 0) return NULL;

    field_t *fields = malloc(field_count * sizeof(field_t));
    u2 offset = 0;
    for(int i = 0; i < field_count; i++) { 
        field_t field;
        field.access_flags = read_u2(file);
        field.name_index = read_u2(file);
        field.descriptor_index = read_u2(file);
        field.attributes_count = read_u2(file);
        field.attributes = read_attributes(file, field.attributes_count, cp_pools);

        field.slot_offset_in_class = offset++;
        field.slot_count = slot_count_from_desciptor(get_utf8(&cp_pools[field.descriptor_index]));
        offset += field.slot_count;
        fields[i] = field;
    }

    return fields;
}




void field_free(field_t *fields, u2 field_count) {
    // todo free
}