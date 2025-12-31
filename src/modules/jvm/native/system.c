#include "native.h"
#include "system.h"
#include "../classfile/class_reader.h"
#include "../classfile/field.h"
#include "../utils/jtype.h"
#include "../runtime/frame.h"
#include "../runtime/operand_stack.h"
#include "../runtime/local_vars.h"

class_t *fake_system_class() { 
    class_t *class = malloc(sizeof(class_t));
    class->class_name = "java/lang/System";
    class->state = CLASS_INITIALIZED;
    class->fields_count = 1;
    field_t *out = malloc(sizeof(field_t));
    out->name = "out";
    out->descriptor = "Ljava/io/PrintStream;";
    out->access_flags = FIELD_ACC_PUBLIC | FIELD_ACC_STATIC;
    out->slot_offset_in_class = 0;
    out->slot_count = 1;
    slot_t *out_slot = malloc(sizeof(slot_t));
    out_slot->ref = malloc(sizeof(object_t));
    out_slot->ref->class = fake_printstream_class();
    out_slot->ref->acount = 0;
    out_slot->ref->fields = NULL;
    out_slot->bits = 0;
    out->init_value = out_slot;
    class->fields = malloc(sizeof(field_t));
    class->fields[0] = *out;
    
    return class;
}

void println_int(frame_t *frame) {
    int32_t val = (int32_t)get_local(frame, 1)->bits;
    printf("%d\n", val);
}

class_t *fake_printstream_class() {
    class_t *class = malloc(sizeof(class_t));
    class->class_name = "java/io/PrintStream";
    class->state = CLASS_INITIALIZED;
    method_t *method = malloc(sizeof(method_t));
    method->name = "println";
    method->descriptor = "(I)V";
    method->access_flags = METHOD_ACC_PUBLIC | METHOD_ACC_NATIVE | METHOD_ACC_FINAL;
    method->arg_slot_count = 1; // this + int parameter
    register_native(class->class_name, method->name, method->descriptor, println_int);
    class->methods = malloc(sizeof(method_t));
    class->methods[0] = *method;
    class->methods_count = 1;
    return class;
}