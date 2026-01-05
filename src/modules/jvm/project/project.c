#include "project.h"
#include "../../../core/list/arraylist.h"
#include "../classfile/class_reader.h"
#include "../../../core/utils.h"
#include "../utils/miniz.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int is_class_file(const char *name) {
    size_t len = strlen(name);
    return len > 6 && strcmp(name + len - 6, ".class") == 0;
}

project_t *load_project(const char *root_path) {
    DIR *dir = opendir(root_path);
    if(!dir) {
        perror("open root dir failed");
        abort();
    }

    project_t *project = malloc(sizeof(project_t));
    struct dirent *entry;
    char full_path[1024];

    project->root_path = strdup(root_path);
    project->class_file_path = arraylist_new(10);

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if(!is_class_file(entry->d_name)) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", root_path, entry->d_name);
        struct stat st;
        if(stat(full_path, &st) != 0){
            perror("stat failed");
            abort();
        }

        if(S_ISREG(st.st_mode)) {
            arraylist_add(project->class_file_path, strdup(full_path));
        }

    }



    return project;
}

void jdk_load(project_t *project, const char *jdk_root) {
    char buffer[1024];
    mz_zip_archive zip;

    sprintf(buffer, "%s/jmods/java.base.jmod", jdk_root);
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, buffer, 0)) {
        fprintf(stderr, "open jmod failed");
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

        // st.m_filename 示例：
        // classes/java/lang/Object.class
        char *filename = strdup(st.m_filename);
        size_t size;
        void* data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        class_bytes_t *class_bytes = class_bytes_new((u1*)data, size);
    }
}


void project_free(project_t *project) {
    // todo free
}