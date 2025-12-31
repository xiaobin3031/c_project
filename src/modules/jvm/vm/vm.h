#pragma once

#include "../classfile/method_info.h"
#include "../classfile/class_reader.h"
#include "../project/project.h"
#include "../runtime/frame.h"

method_t *resolve_method(class_t *class, const char *method_name, const char *method_descriptor);

class_t *load_class(const char *class_file, jvm_thread_t *thread);
void link_class(class_t *class);
void init_class(class_t *class, jvm_thread_t *thread);

void prepare_run(jvm_thread_t *thread);

void run(const char *main_class_file, project_t *project);