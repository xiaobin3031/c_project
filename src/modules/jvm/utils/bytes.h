#pragma once

#include "jtype.h"
#include <stdio.h>
#include <stdint.h>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;

u1 read_u1(FILE *class_file);
u2 read_u2(FILE *class_file);
u4 read_u4(FILE *class_file);

u2 parse_to_u2(u1 *bytes);
u4 parse_to_u4(u1 *bytes);

u1 *read_bytes(FILE *class_file, u4 len);

u1 *parse_bytes(u1 *src, u4 len);

u1 read_bytes_u1(class_bytes_t *class_bytes);

u2 read_bytes_u2(class_bytes_t *class_bytes);

u4 read_bytes_u4(class_bytes_t *class_bytes);

u4 read_bytes_bytes(class_bytes_t *class_bytes, u4 len);