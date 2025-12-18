#include "vm.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../interpreter/interpreter.h"


void run(method_t *method, class_t *class) {
    if(method->attributes_count > 0) {
        frame_t *frame = create_frame(method, NULL);
        interpret(frame, class);
        frame_free(frame);
    }
}