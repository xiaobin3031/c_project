#include "class_bytes.h"
#include "../utils/bytes.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

u1 read_bytes_u1(class_bytes_t *class_bytes) {
    if(class_bytes->offset >= class_bytes->size) {
        fprintf(stderr, "class bytes out of range\n");
        abort();
    }
    return class_bytes->bytes[class_bytes->offset++];
}

u2 read_bytes_u2(class_bytes_t *class_bytes) {
    u2 high = read_bytes_u1(class_bytes);
    u2 low = read_bytes_u1(class_bytes);
    return (high << 8) | low;
}

u4 read_bytes_u4(class_bytes_t *class_bytes) {
    return ((u4)read_bytes_u1(class_bytes) << 24)
        | ((u4)read_bytes_u1(class_bytes) << 16)
        | ((u4)read_bytes_u1(class_bytes) << 8) 
        | (u4)read_bytes_u1(class_bytes);
}

u1 *read_bytes_bytes(class_bytes_t *class_bytes, u4 len) {
    if(len == 0) return NULL;
    u1 *bytes = malloc(len);
    if(class_bytes->offset + len > class_bytes->size) {
        fprintf(stderr, "read_bytes failed\n");
        abort();
    }
    memcpy(bytes, class_bytes->bytes + class_bytes->offset, len);
    class_bytes->offset += len;
    return bytes;
}

class_bytes_t *class_bytes_new(u1 *bytes, size_t len) {
    char *memory = malloc(sizeof(class_bytes_t) + sizeof(u1) * len);
    class_bytes_t *class_bytes = (class_bytes_t *)memory;
    class_bytes->bytes = (u1*)(memory + sizeof(class_bytes_t));
    class_bytes->offset = 0;
    return class_bytes;
}