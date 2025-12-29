#include "local_vars.h"
#include "../utils/jtype.h"
#include "frame.h"

slot_t *get_local(frame_t *frame, u2 index) {
    if(index >= frame->local_var_size) {
        fprintf(stderr, "local var index out of range: %d\n", index);
        dump_frame(frame);
        abort();
    }
    return &frame->local_vars[index];
}