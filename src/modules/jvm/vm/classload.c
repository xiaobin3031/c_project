#include "classload.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../interpreter/interpreter.h"
#include "../project/project.h"
#include "../../../core/list/arraylist.h"
#include "../../../core/utils.h"
#include "../utils/miniz.h"
#include "../runtime/class.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static project_t *g_project;
static arraylist *g_class_list;

const char* atype_to_descriptor(u1 atype) {
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
    // fprintf(stderr, "cannot find method: %s %s in class %s\n", method_name, method_descriptor, class->class_name);
    // abort();
}

class_t *load_class(const char *class_name, jvm_thread_t *thread) {
    // load from cache
    class_t *class = find_class(class_name);
    if(class == NULL) {
        // printf("load class name: %s, g_class_list.size: %ld\n", class_file, g_class_list->size);
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s.class", g_project->root_path, class_name);
        class_file_t *class_file = read_class_file(full_path);
        class = define_class(class_file);
        if(class) {
            arraylist_add(g_class_list, class);
            // if(class->major_version != 61) {
            //     perror("UnsupportedClassVersionError");
            //     abort();
            // }
            if(strcmp(class->class_name, class_name) != 0) {
                fprintf(stderr, "NoClassDefFoundError: %s\n", class_file);
                abort();
            }
            class->state = CLASS_LOADED;
        }
    }
    if(class == NULL){
        fprintf(stderr, "ClassFormatError: %s\n", class_name);
        abort();
    }
    if(class->state == CLASS_ERRONEOUS) {
        thread->error = error_new(RUNTIME_ERROR_NoClassDefFoundError, "Class is in erroneous state");
    }else{
        link_class(thread, class);
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
        array_class->interface_class[1] = load_class("java/lang/Serializable", thread);
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
        frame_t *clinit_frame = frame_new(clinit, NULL, class);
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

void load_jdk_class(project_t *project, jvm_thread_t *thread, const char *class_name) { 
}

static int is_class_match(const char *file_name, const char *class_name) {
    size_t class_name_len = strlen(class_name);
    const char *ptr = file_name;
    while(*ptr != '\0' && strlen(ptr) >= class_name_len) {
        if(strncmp(ptr, class_name, class_name_len) == 0) {
            // 匹配
            return 0;
        }
        ptr++;
    }
    return 1;
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

    char buffer[1024];
    mz_zip_archive zip;

    sprintf(buffer, "%s/jmods/java.base.jmod", project->jdk_root);
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, buffer, 0)) {
        fprintf(stderr, "open jmod failed\n");
        return;
    }

    // 获取文件总数
    mz_uint num = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < num; i++) {
        mz_zip_archive_file_stat st;

        if (!mz_zip_reader_file_stat(&zip, i, &st))
            continue;

        if (st.m_is_directory)
            continue;

        if (!start_with(st.m_filename, "classes/"))
            continue;

        if (!end_with(st.m_filename, ".class"))
            continue;
        
        if(is_class_match(st.m_filename, "java/io/FilterOutputStream") == 0) {
            printf("match jdk class: %s\n", st.m_filename);
        }

        char *filename = strdup(st.m_filename);
        size_t size;
        void* data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        // printf("load class file: %s, size: %ld\n", filename, size);
        class_file_bytes_t *class_bytes = class_bytes_new((u1*)data, size);
        class_file_t *class = read_by_class_bytes(class_bytes);
        // printf("loaded class file: %s\n", class->class_name);
        arraylist_add(g_class_list, class);
        free(filename);
        free(class_bytes);
        free(data);
    }

    if(g_class_list->size > 0) {
        for(size_t i = 0; i < g_class_list->size; i++) {
            class_t *class = arraylist_get(g_class_list, i);
            int is_white = 0;
            for(int i=0;white_classes[i] != NULL;i++) {
                if(strcmp(class->class_name, white_classes[i]) == 0) {
                    is_white = 1;
                    break;
                }
            }
            if(is_white == 0) continue;

            link_class(main_thread, class);
        }
    }
}