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
#include <pthread.h>

static project_t *g_project;
static arraylist *g_class_list;

/**
 * 这个方法不一定有，如果没有就返回空，没必要报错
 */
static method_t *resolve_clinit(class_t *class) {
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = &class->methods[i];
        char *m_name = get_utf8(&class->cp_pools[method->name_index]);
        char *m_descriptor = get_utf8(&class->cp_pools[method->descriptor_index]);
        if(strcmp(m_name, "<clinit>") == 0 && strcmp(m_descriptor, "()V") == 0) {
            return method;
        }
    }
    return NULL;
}

method_t *resolve_method(class_t *class, const char *method_name, const char *method_descriptor) {
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = &class->methods[i];
        char *m_name = get_utf8(&class->cp_pools[method->name_index]);
        char *m_descriptor = get_utf8(&class->cp_pools[method->descriptor_index]);
        printf("m_name: %s, m_descriptor: %s\n", m_name, m_descriptor);
        if(strcmp(m_name, method_name) == 0 && strcmp(m_descriptor, method_descriptor) == 0) {
            return method;
        }
    }
    fprintf(stderr, "cannot find method: %s %s in class %s\n", method_name, method_descriptor, class->class_name);
    abort();
}

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
    if(class->state >= CLASS_LINKED) {
        return;
    }
    // todo verify
}

void init_class(class_t *class) {
    if(class->state >= CLASS_INITIALIZED) {
        return;
    }
    pthread_mutex_lock(&class->lock);
    if(class->state >= CLASS_INITIALIZED) {
        pthread_mutex_unlock(&class->lock);
        return;
    }
    // todo clinit

    class->state = CLASS_INITING;

    if(class->super_class != 0) {
        cp_info_t super_class_info = class->cp_pools[class->super_class];
        check_cp_info_tag(super_class_info.tag, CONSTANT_Class);
        char *super_class_name = get_utf8(&class->cp_pools[parse_to_u2(super_class_info.info)]);
        if(strcmp(super_class_name, "java/lang/Object") != 0) 
            load_class(super_class_name);
    }

    method_t *clinit = resolve_clinit(class);
    if(clinit) {
        frame_t *clinit_frame = frame_new(clinit, NULL);
        interpret(clinit_frame, class);
    }

    pthread_mutex_unlock(&class->lock);
}

void run(const char *main_class_file, project_t *project) {
    g_project = project;
    g_class_list = arraylist_new(10);

    class_t *main_class = load_class(main_class_file);
    method_t *main_method = resolve_method(main_class, "main", "([Ljava/lang/String;)V");
    frame_t *frame = frame_new(main_method, NULL);
    interpret(frame, main_class);
    frame_free(frame);
}