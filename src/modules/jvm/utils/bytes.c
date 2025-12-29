#include "bytes.h"
#include <stdio.h>
#include <stdlib.h>

u1 read_u1(FILE *class_file) {
    u1 r;
    if (fread(&r, 1, 1, class_file) != 1) {
        fprintf(stderr, "read_u1 failed\n");
        abort();
    }
    return r;
}

u2 read_u2(FILE *class_file) {
    u2 high = read_u1(class_file);
    u2 low = read_u1(class_file);
    return (high << 8) | low;
}

u4 read_u4(FILE *class_file) {
    return ((u4)read_u1(class_file) << 24)
        | ((u4)read_u1(class_file) << 16)
        | ((u4)read_u1(class_file) << 8) 
        | (u4)read_u1(class_file);
}


u2 parse_to_u2(u1 *bytes) {
    return ((u2)bytes[0] << 8) | bytes[1];
}

u4 parse_to_u4(u1 *bytes) {
    return ((u4)bytes[0] << 24)
        | ((u4)bytes[1] << 16)
        | ((u4)bytes[2] << 8)
        | (u4)bytes[3];
}

u1 *read_bytes(FILE *class_file, u4 len) {
    if(len == 0) return NULL;
    u1 *bytes = malloc(len);
    if (fread(bytes, 1, len, class_file) != len) {
        fprintf(stderr, "read_bytes failed\n");
        abort();
    }
    return bytes;
}