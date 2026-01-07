#pragma once

#include "../classfile/method_info.h"
#include "../classfile/class_reader.h"
#include "../project/project.h"
#include "../runtime/frame.h"

method_t *find_method(class_t *class, const char *method_name, const char *method_descriptor);

class_t *load_class(const char *class_file, jvm_thread_t *thread);

class_t *load_array_class(jvm_thread_t *thread, u1 type);

void link_class(jvm_thread_t *thread, class_t *class);

void ensure_class_initialized(class_t *class, jvm_thread_t *thread);

void load_jdk_class(project_t *project, jvm_thread_t *thread, const char *class_name);

void bootstrap(project_t *project);
