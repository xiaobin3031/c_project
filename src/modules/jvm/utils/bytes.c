#include "bytes.h"
#include <stdio.h>
#include <stdlib.h>

u1 read_u1(FILE *class_file) {
    u1 r;
    if (fread(&r, 1, 1, class_file) != 1) {
        fprintf(stderr, "read_u1 failed\n");
        exit(1);
    }
    return r;
}

u2 read_u2(FILE *class_file) {
    u1 high = read_u1(class_file);
    u1 low = read_u1(class_file);
    return (high << 8) | low;
}

u4 read_u4(FILE *class_file) {
    return (read_u1(class_file) << 24)
        | (read_u1(class_file) << 16)
        | (read_u1(class_file) << 8) 
        | read_u1(class_file);
}