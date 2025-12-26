#include "vm.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../interpreter/interpreter.h"
#include "../project/project.h"
#include "../../../core/list/arraylist.h"
#include <stdio.h>
#include <string.h>

static project_t *g_project;
static arraylist *g_class_list;

class_t *load_class(const char *class_file) {
    // load from cache
    class_t *class = NULL;
    for(size_t i=0;i<g_class_list->size;i++) {
        class_t *tmp = (class_t*)arraylist_get(g_class_list, i);
        if(strcmp(tmp->class_name, class_file) == 0) {
            class = tmp;
            break;
        }
    }
    if(class) return class;

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s.class", g_project->root_path, class_file);
    class = read_class_file(full_path);
    if(class) {
        link_class(class);
        init_class(class);

        arraylist_add(g_project->class_file_path, class);
        return class;
    }

    // todo 这里后续要删掉
    if(strcmp(class_file, "java/lang/String") == 0) {
        return calloc(1, sizeof(class_t));
    }

    fprintf(stderr, "class file not found: %s\n", class_file);
    abort();
}

void link_class(class_t *class) { 
}

void init_class(class_t *class) {
}

void run(const char *main_class_file, project_t *project) {
    g_project = project;
    g_class_list = arraylist_new(10);

    class_t *main_class = load_class(main_class_file);
    method_t *main_method;
    for(int i=0;i<main_class->methods_count;i++) {
        method_t method = main_class->methods[i];
        char *method_name = get_utf8(&main_class->cp_pools[method.name_index]);
        if(strcmp(method_name, "main") == 0) {
            main_method = &method;
            break;
        }
    }
    if(main_method == NULL){
        printf("Can not find main method in class %s\n", main_class_file);
        abort();
    }
    frame_t *frame = create_frame(main_method, NULL);
    interpret(frame, main_class);
    frame_free(frame);
}