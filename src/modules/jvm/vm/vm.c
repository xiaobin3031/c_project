#include "vm.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../interpreter/interpreter.h"
#include "../project/project.h"
#include "classload.h"
#include "../native/native.h"
#include <stdio.h>

void run(const char *main_class_file, project_t *project) {

    register_native_methods();

    bootstrap(project);

    jvm_thread_t *main_thread = jvm_thread_new();
    class_file_t *main_class = load_class(main_class_file, main_thread);
    ensure_class_initialized(main_class, main_thread);
    method_file_t *main_method = resolve_method(main_class, "main", "([Ljava/lang/String;)V");
    frame_t *frame = frame_new(main_method, NULL, main_class);

    main_thread->current_frame = NULL;
    push_frame(main_thread, frame);
    interpret(main_thread);
    if(main_thread->error != NULL) {
        fprintf(stderr, "Uncaught exception: %s\n", main_thread->error->message);
        fprintf(stderr, "Program exit by Uncaught exception\n");
        exit(1);
    }
}