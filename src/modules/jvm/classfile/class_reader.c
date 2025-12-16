#include "class_reader.h"
#include "../utils/bytes.h"
#include <stdio.h>
#include <stdlib.h>

class_t *read_class_file(const char *path) {
    FILE *class_file;

    class_file = fopen(path, "rb");
    if(!class_file) {
        perror("class file read error");
        return NULL;
    }

    class_t *class = malloc(sizeof(class_t));
    class->magic = read_u4(class_file);
    class->minor_version = read_u2(class_file);
    class->major_version = read_u2(class_file);

    fclose(class_file);

    return class;
}






void class_free(class_t *class) {
    if(class) {
        free(class);
    }
}