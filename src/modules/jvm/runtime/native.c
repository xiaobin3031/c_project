#include "native.h"
#include "../../../core/list/arraylist.h"
#include "operand_stack.h"
#include "local_vars.h"
#include "jmemory.h"
#include "../vm/classload.h"
#include "class.h"
#include <string.h>


static arraylist *native_methods = NULL;

void register_native(
    const char *class_name,
    const char *method_name,
    const char *descriptor,
    native_fn fn
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

native_fn find_native_method(
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
    // todo 找不到native方法
    fprintf(stderr, "native method not found: %s %s %s\n", class_name, method_name, descriptor);
    abort();
}

void java_lang_object_getClass(jvm_thread_t *thread, frame_t *frame) {
    slot_t *slot = get_local(frame, 0);
    push(frame)->ref = slot->ref;
}

void java_lang_system_registerNatives(jvm_thread_t *thread, frame_t *frame) {
    // todo
}
void java_lang_class_registerNatives(jvm_thread_t *thread, frame_t *frame) {
    // todo
}

void java_lang_class_initClassName(jvm_thread_t *thread, frame_t *frame) {
    object_t *ref = heap_alloc_object(load_class("java/lang/String", thread));
    const char *name = frame->method->klass->class_name;
    native_string_t *string = string_new(name);
    ref->native_string = string;
    push(frame)->ref = ref;
}

void register_native_methods() { 
    // java/lang/Object
    register_native("java/lang/Object", "getClass", "()Ljava/lang/Class;", java_lang_object_getClass);

    register_native("java/lang/Class", "registerNatives", "()V", java_lang_class_registerNatives);
    register_native("java/lang/Class", "initClassName", "()Ljava/lang/String;", java_lang_class_initClassName);

    // java/lang/System
    register_native("java/lang/System", "registerNatives", "()V", java_lang_system_registerNatives);
}