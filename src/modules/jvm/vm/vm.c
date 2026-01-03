#include "vm.h"
#include "../classfile/method_info.h"
#include "../runtime/frame.h"
#include "../classfile/attr.h"
#include "../classfile/class_reader.h"
#include "../interpreter/interpreter.h"
#include "../project/project.h"
#include "../../../core/list/arraylist.h"
#include "../native/system.h"
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
        if(strcmp(method->name, "<clinit>") == 0 && strcmp(method->descriptor, "()V") == 0) {
            return method;
        }
    }
    return NULL;
}

static char *resolve_class_name(class_t *class, u2 class_index){ 
    cp_info_t cp_info = class->cp_pools[class_index];
    check_cp_info_tag(cp_info.tag, CONSTANT_Class);
    cp_class_t *cp_class = (cp_class_t *)cp_info.info;
    return get_utf8(&class->cp_pools[cp_class->name_index]);
}

/**
 * 查询自己类中的field，用于初始化
 */
static field_t *find_field(class_t *class, cp_info_t *cp_info) {
    cp_fieldref_t *fieldref = (cp_fieldref_t *)cp_info->info;
    cp_info_t local_cp_info = class->cp_pools[fieldref->name_and_type_index];
    check_cp_info_tag(local_cp_info.tag, CONSTANT_NameAndType);
    cp_nameandtype_t *nametype = (cp_nameandtype_t *)local_cp_info.info;
    char *name = get_utf8(&class->cp_pools[nametype->name_index]);
    char *desc = get_utf8(&class->cp_pools[nametype->descriptor_index]);
    for(u2 i = 0; i < class->fields_count; i++) { 
        field_t *field = &class->fields[i];
        if(strcmp(name, field->name) == 0 && strcmp(desc, field->descriptor) == 0) {
            return field;
        }
    }

    return NULL;
}

method_t *resolve_method(class_t *class, const char *method_name, const char *method_descriptor) {
    for(int i=0;i<class->methods_count;i++) {
        method_t *method = &class->methods[i];
        printf("m_name: %s, m_descriptor: %s\n", method->name, method->descriptor);
        if(strcmp(method->name, method_name) == 0 && strcmp(method->descriptor, method_descriptor) == 0) {
            return method;
        }
    }
    fprintf(stderr, "cannot find method: %s %s in class %s\n", method_name, method_descriptor, class->class_name);
    abort();
}

class_t *load_class(const char *class_file, jvm_thread_t *thread) {
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
        arraylist_add(g_class_list, class);
        // if(class->major_version != 61) {
        //     perror("UnsupportedClassVersionError");
        //     abort();
        // }
        if(strcmp(class->class_name, class_file) != 0) {
            fprintf(stderr, "NoClassDefFoundError: %s\n", class_file);
            abort();
        }
        link_class(class);
        init_class(class, thread);

        return class;
    }

    // todo 这里后续要删掉
    if(strcmp(class_file, "java/lang/String") == 0 
        || strcmp(class_file, "java/lang/System") == 0
        || strcmp(class_file, "java/lang/RuntimeException") == 0
        || strcmp(class_file, "java/lang/Object") == 0) {
        class_t *class = calloc(1, sizeof(class_t));
        class->class_name = strdup(class_file);
        arraylist_add(g_project->class_file_path, class);
        return class;
    }

    fprintf(stderr, "ClassFormatError: %s\n", class_file);
    abort();
}

void link_class(class_t *class) { 
    if(class->state < CLASS_LOADED) {
        fprintf(stderr, "class %s is not loaded yet\n", class->class_name);
        abort();
    }
    if(class->state >= CLASS_LINKED) {
        return;
    }
    printf("link class %s\n", class->class_name);
    // todo verify
    if(class->access_flags & CLASS_ACC_INTERFACE) {
        // super class必须是object
        if(class->super_class <= 0) {
            fprintf(stderr, "Interface %s's super class must be java/lang/Object\n", class->class_name);
            abort();
        }
        char *super_name = get_utf8(&class->cp_pools[class->super_class]);
        if(strcmp(super_name, "java/lang/Object") != 0) {
            fprintf(stderr, "Interface %s's super class must be java/lang/Object\n", class->class_name);
            abort();
        }
    }

    // create static fields
    cp_info_t *cp_pools = class->cp_pools;
    for(u2 i=1;i<class->constant_pool_count;i++) {
        cp_info_t *cp_info = &cp_pools[i];
        if(cp_info->tag == CONSTANT_Fieldref) {
            field_t *field = find_field(class, cp_info);
            if(field != NULL && field->access_flags & FIELD_ACC_STATIC) {
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
                slot_t *slot = malloc(sizeof(slot_t) * field->slot_count);
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

                // 设置到cp_fieldref中
                field->init_value = slot;
            }
        }
    }

    class->state = CLASS_LINKED;
}

void init_class(class_t *class, jvm_thread_t *thread) {
    if(class->state < CLASS_LINKED) {
        fprintf(stderr, "class %s is not linked yet\n", class->class_name);
        abort();
    }
    if(class->state >= CLASS_INITIALIZED) {
        return;
    }
    pthread_mutex_lock(&class->lock);
    if(class->state >= CLASS_INITIALIZED) {
        pthread_mutex_unlock(&class->lock);
        return;
    }

    class->state = CLASS_INITING;

    if(class->super_class != 0) {
        cp_info_t super_class_info = class->cp_pools[class->super_class];
        check_cp_info_tag(super_class_info.tag, CONSTANT_Class);
        char *super_class_name = resolve_class_name(class, class->super_class);
        if(strcmp(super_class_name, "java/lang/Object") != 0) 
            load_class(super_class_name, thread);
    }

    method_t *clinit = resolve_clinit(class);
    if(clinit) {
        printf("init class %s by clinit\n", class->class_name);
        frame_t *clinit_frame = frame_new(clinit, NULL, class);
        push_frame(thread, clinit_frame);
        interpret(thread);
        printf("finish init class %s\n", class->class_name);
    }

    class->state = CLASS_INITIALIZED;

    pthread_mutex_unlock(&class->lock);
}

void prepare_run(jvm_thread_t *thread) {
    load_class("java/lang/Object", thread);
    class_t *system_class = fake_system_class();
    arraylist_add(g_class_list, system_class);
    class_t *printstream_class = fake_printstream_class();
    arraylist_add(g_class_list, printstream_class);
    load_class("java/lang/String", thread);
    load_class("java/lang/RuntimeException", thread);
}

void run(const char *main_class_file, project_t *project) {
    g_project = project;
    g_class_list = arraylist_new(10);

    jvm_thread_t *main_thread = jvm_thread_new();
    prepare_run(main_thread);

    class_t *main_class = load_class(main_class_file, main_thread);
    method_t *main_method = resolve_method(main_class, "main", "([Ljava/lang/String;)V");
    frame_t *frame = frame_new(main_method, NULL, main_class);

    main_thread->current_frame = NULL;
    push_frame(main_thread, frame);
    interpret(main_thread);
}