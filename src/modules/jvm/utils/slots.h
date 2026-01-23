#pragma once

#include "bytes.h"

u2 slot_count_from_desciptor(char *descriptor);

/**
 * 参数个数，不考虑Long和Double占两个的逻辑
 */
u2 arg_count_from_desciptor(char *descriptor);

char *descriptor_to_simple_type(const char *descriptor);

char *descriptor_to_type(const char *descriptor);