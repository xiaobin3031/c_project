#pragma once

#include "../../../core/list/arraylist.h"

typedef struct {
    char *root_path;
    // jdk目录
    char *jdk_root;
    arraylist *class_file_path;
} project_t;

int is_class_file(const char *name);

project_t *load_project(const char *root_path);

void project_free(project_t *project);