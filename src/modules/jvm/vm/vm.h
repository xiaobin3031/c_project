#pragma once

#include "../classfile/method_info.h"
#include "../classfile/class_reader.h"
#include "../project/project.h"

class_t *load_class(const char *class_file);
void link_class(class_t *class);
void init_class(class_t *class);

void run(const char *main_class_file, project_t *project);