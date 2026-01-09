#include "classload.h"
#include "../runtime/frame.h"
#include "../runtime/class.h"
#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../classfile/method_info.h"
#include "../interpreter/interpreter.h"
#include "../project/project.h"
#include "../../../core/list/arraylist.h"
#include "../utils/miniz.h"
#include "../utils/slots.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static project_t *g_project;
static arraylist *g_class_list;

static const char* atype_to_descriptor(u1 atype) {
    switch (atype) {
        case 4:  return "Z";
        case 5:  return "C";
        case 6:  return "F";
        case 7:  return "D";
        case 8:  return "B";
        case 9:  return "S";
        case 10: return "I";
        case 11: return "J";
        default: return NULL; // 抛 VerifyError
    }
}

static class_t *find_class(const char *class_name) {
    if(g_class_list == NULL) {
        g_class_list = arraylist_new(100);
    }
    class_t *class = NULL;
    for(size_t i=0;i<g_class_list->size;i++) {
        class_t *tmp = (class_t*)arraylist_get(g_class_list, i);
        if(tmp != NULL && tmp->class_name != NULL 
            && strcmp(tmp->class_name, class_name) == 0) {
            class = tmp;
            break;
        }
    }
    return class;
}

method_t *find_method(class_t *class, const char *method_name, const char *method_descriptor) {
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = &class->methods[i];
        if(strcmp(method->name, method_name) == 0 && strcmp(method->descriptor, method_descriptor) == 0) {
            return method;
        }
    }
    return NULL;
}

class_t *load_class(const char *class_name, jvm_thread_t *thread) {
    // load from cache
    class_t *class = find_class(class_name);
    if(class == NULL) {
        class = calloc(1, sizeof(class_t));
        class->class_name = strdup(class_name);
        class->class_simple_name = descriptor_to_simple_type(class_name);
        arraylist_add(g_class_list, class);
        class->state = CLASS_LOADED;
    }
    
    return class;

}

class_t *load_array_class(jvm_thread_t *thread, u1 type) {
    const char *descriptor = atype_to_descriptor(type);
    if(descriptor == NULL) return NULL;

    class_t *array_class = find_class(descriptor);
    if(array_class == NULL) {
        array_class = calloc(1, sizeof(class_t));
        array_class->is_array = 1;
        array_class->class_name = strdup(descriptor);
        array_class->super = load_class("java/lang/Object", thread);
        array_class->interface_class = calloc(2, sizeof(class_t*));
        array_class->interface_class[0] = load_class("java/lang/Cloneable", thread);
        array_class->interface_class[1] = load_class("java/io/Serializable", thread);
        array_class->interface_count = 2;

        arraylist_add(g_class_list, array_class);
    }

    return array_class;

}

void link_class(jvm_thread_t *thread, class_t *class) { 
    if(class->state < CLASS_LOADED) {
        fprintf(stderr, "class %s is not loaded yet\n", class->class_name);
        abort();
    }
    if(class->state >= CLASS_LINKED) {
        return;
    }
    printf("link class %s\n", class->class_name);
    // todo verify
    // todo 接口也能继承了，文档读的有问题，后续再补

    // create static fields
    if(class->fields_count > 0) {
        for(u2 i=0;i<class->fields_count;i++) {
            field_t *field = &class->fields[i];
            if(field->access_flags & FIELD_ACC_STATIC) {
                // 初始化静态字段
                if(*field->descriptor == 'J' || *field->descriptor == 'D') {
                    if(field->slot_count != 2) {
                        fprintf(stderr, "field %s descriptor %s slot count mismatch\n", field->name, field->descriptor);
                        abort();
                    }
                }else {
                    if(field->slot_count != 1) {
                        fprintf(stderr, "field %s descriptor %s slot count mismatch\n", field->name, field->descriptor);
                        abort();
                    }
                }

                slot_t *slot = field->slots;
                // todo 初始化slot
                switch(*field->descriptor) {
                    case 'D':
                    case 'J':
                        slot[0].bits = 0;
                        slot[0].ref = NULL;
                        slot[1].bits = 0;
                        slot[1].ref = NULL;
                        break;
                    default:
                        slot->bits = 0;
                        slot->ref = NULL;
                        break;
                }
            }

        }
    }

    if(class->super_class_name != NULL) {
        class->super = load_class(class->super_class_name, thread);
        free(class->super_class_name);
        class->super_class_name = NULL;
    }

    if(class->interface_class_names != NULL) {
        class->interface_class = calloc(class->interface_count, sizeof(class_t *));
        for(int i = 0; i < class->interface_count; i++) {
            class->interface_class[i] = load_class(class->interface_class_names[i], thread);
            free(class->interface_class_names[i]);
        }
        free(class->interface_class_names);
    }

    class->state = CLASS_LINKED;
}

void ensure_class_initialized(class_t *class, jvm_thread_t *thread) {
    if(class->state < CLASS_LINKED) {
        if(class->state == CLASS_LOADED) {
            link_class(thread, class);
        }
        if(class->state < CLASS_LINKED) {
            fprintf(stderr, "class %s is not linked yet, state: %d\n", class->class_name, class->state);
            abort();
        }
    }
    if(class->state >= CLASS_INITING) {
        return;
    }
    if(class->state == CLASS_ERRONEOUS) {
        thread->error = error_new(RUNTIME_ERROR_NoClassDefFoundError, "Class is in erroneous state");
        return;
    }

    pthread_mutex_lock(&class->lock);
    if(class->state >= CLASS_INITIALIZED) {
        pthread_mutex_unlock(&class->lock);
        return;
    }

    class->state = CLASS_INITING;

    if(class->super != NULL) {
        ensure_class_initialized(class->super, thread);
    }
    if(class->interface_count > 0) {
        printf("interface count: %d\n", class->interface_count);
        for(u2 i=0;i<class->interface_count;i++) {
            ensure_class_initialized(class->interface_class[i], thread);
        }
    }

    method_t *clinit = find_method(class, "<clinit>", "()V");
    if(clinit) {
        jvm_thread_t *clinit_thread = jvm_thread_new();
        printf("init class %s by clinit\n", class->class_name);
        frame_t *clinit_frame = frame_new(clinit, NULL);
        push_frame(clinit_thread, clinit_frame);
        interpret(clinit_thread);
        error_t *error = clinit_thread->error;
        free(clinit_thread);
        if(error != NULL) {
            class->state = CLASS_ERRONEOUS;
            error->type = RUNTIME_ERROR_ExceptionInInitializerError;
            thread->error = error;
            pthread_mutex_unlock(&class->lock);
            return;
        }
    }

    class->state = CLASS_INITIALIZED;
    pthread_mutex_unlock(&class->lock);
}

void bootstrap(project_t *project) {
    g_project = project;
    g_class_list = arraylist_new(100);

    const char *white_classes[] = {
        "java/lang/Object",
        "java/lang/String",
        "java/lang/System",
        // "java/io/PrintStream",
        // "java/io/FilterOutputStream",
        // "java/io/OutputStream",
        // "java/io/Closeable",
        // "java/io/Flushable",
        "java/io/Serializable",
        // "java/lang/AutoCloseable",
        "java/lang/Throwable",
        // "java/lang/Appendable",
        "java/lang/Exception",
        "java/lang/RuntimeException",
        NULL
    };

    jvm_thread_t *main_thread = jvm_thread_new();

    for (int i = 0; white_classes[i] != NULL; i++) {
        load_class(white_classes[i], main_thread);
    }
}

void add_class(class_t *klass) {
    if(find_class(klass->class_name) == NULL) {
        arraylist_add(g_class_list, klass);
    }
}