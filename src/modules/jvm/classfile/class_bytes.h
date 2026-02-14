#pragma once

#include "../utils/bytes.h"
#include <stdio.h>

typedef struct {
    u1 *bytes;
    size_t offset;
    size_t size;
} class_file_bytes_t;

u1 read_bytes_u1(class_file_bytes_t *class_bytes);

u2 read_bytes_u2(class_file_bytes_t *class_bytes);

u4 read_bytes_u4(class_file_bytes_t *class_bytes);

u1 *read_bytes_bytes(class_file_bytes_t *class_bytes, u4 len);

class_file_bytes_t *class_bytes_new(u1 *bytes, size_t len);