#pragma once

#include "../../../core/list/arraylist.h"
#include "../utils/miniz.h"

typedef enum {
    CLASS_FILE_SOURCE_FILE,
    CLASS_FILE_SOURCE_JMOD
} class_file_source_e ;

typedef struct {
    char *name;
    class_file_source_e source;
    union {
        mz_uint index;
        char *path;
    };
} class_file_source_t;
typedef struct {
    char *root_path;
    // jdk目录
    char *jdk_root;
    arraylist *class_file_path;


    // jmods
    mz_zip_archive *jmod_base_zip;

    arraylist *class_file_source;
} project_t;

int is_class_file(const char *name);

project_t *load_project(const char *root_path, const char *jdk_home);

void project_free(project_t *project);