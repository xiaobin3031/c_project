#include "field.h"
#include "../utils/bytes.h"
#include "attr.h"
#include <stdlib.h>

field_t **read_fields(FILE *file, u2 field_count, void **cp_pools) {
    if(field_count == 0) return NULL;

    field_t **fields = malloc(field_count * sizeof(field_t *));
    for(int i = 0; i < field_count; i++) { 
        field_t *field = malloc(sizeof(field_t));
        field->access_flags = read_u2(file);
        field->name_index = read_u2(file);
        field->descriptor_index = read_u2(file);
        field->attributes_count = read_u2(file);
        field->attributes = read_attributes(file, field->attributes_count, cp_pools);
        fields[i] = field;
    }

    return fields;
}




void field_free(field_t **fields, u2 field_count) {
    if(fields) {
        for(int i = 0; i < field_count; i++) {
            field_t *field = fields[i];
            if(!field) continue;

            attr_free(field->attributes, field->attributes_count);
            free(field);
        }
        free(fields);
    }
}