#include "vm.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../classfile/attr.h"
#include "../classfile/constant_pool.h"
#include "../interpreter/interpreter.h"


void run(method_t *method, void **cp_pools) {
    if(method->attributes_count > 0) {
        code_attr_t *code;
        for(int i=0;i<method->attributes_count;i++) {
            void *info = method->attributes[i];
            if(is_cp_info_tag(info, ATTR_CODE)) {
                code = (code_attr_t *)info;
                break;
            }
        }
        if(!code) return;

        frame_t *frame = frame_new(code);
        interpret(frame, cp_pools);
        frame_free(frame);
    }
}