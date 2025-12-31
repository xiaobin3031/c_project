#pragma once
#include "../runtime/frame.h"

typedef void (*native_fn)(frame_t*);


typedef struct {
    char *class_name;
    char *method_name;
    char *descriptor;
    native_fn fn;
} native_method_t;

void register_native(
    const char *class_name,
    const char *method_name,
    const char *descriptor,
    native_fn fn
);

native_fn find_native_method(
    const char *class_name,
    const char *method_name,
    const char *descriptor
);