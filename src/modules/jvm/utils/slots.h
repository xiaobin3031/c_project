#pragma once

#include "bytes.h"

u2 slot_count_from_desciptor(char *descriptor);


char *descriptor_to_simple_type(const char *descriptor);

char *descriptor_to_type(const char *descriptor);