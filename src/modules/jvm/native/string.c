#include "string.h"
#include "../utils/jtype.h"
#include "../vm/vm.h"
#include <string.h>

object_t *new_string_object(const char *str) { 
    object_t *ref = calloc(1, sizeof(object_t));
    ref->class = load_class("java/lang/String");
    ref->strings = strdup(str);
    return ref;
}