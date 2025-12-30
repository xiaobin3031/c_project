#include "native.h"
#include "../../../core/list/arraylist.h"


static arraylist *native_methods = NULL;

void register_native(
    const char *class_name,
    const char *method_name,
    const char *descriptor,
    native_fn *fn
) {
    if(native_methods == NULL) {
        native_methods = arraylist_new(10);
    }

    native_method_t *native_method = malloc(sizeof(native_method_t));
    native_method->class_name = strdup(class_name);
    native_method->method_name = strdup(method_name);
    native_method->descriptor = strdup(descriptor);
    native_method->fn = fn;
    arraylist_add(native_methods, native_method);
}

native_fn *find_native_method(
    const char *class_name,
    const char *method_name,
    const char *descriptor
) { 
    if(native_methods == NULL) {
        return NULL;
    }

    for(size_t i=0;i<native_methods->size;i++) {
        native_method_t *native_method = (native_method_t *)arraylist_get(native_methods, i);
        if(strcmp(native_method->class_name, class_name) == 0 &&
           strcmp(native_method->method_name, method_name) == 0 &&
           strcmp(native_method->descriptor, descriptor) == 0) {
            return native_method->fn;
        }
    }
    return NULL;
}