#include "project.h"
#include "../../../core/list/arraylist.h"
#include "../classfile/class_reader.h"
#include "../../../core/utils.h"
#include "../utils/miniz.h"
#include "../classfile/class_bytes.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int is_class_file(const char *name) {
    size_t len = strlen(name);
    return len > 6 && strcmp(name + len - 6, ".class") == 0;
}

project_t *load_project(const char *root_path, const char *jdk_home) {
    DIR *dir = opendir(root_path);
    if(!dir) {
        perror("open root dir failed");
        abort();
    }

    project_t *project = calloc(1, sizeof(project_t));
    struct dirent *entry;
    char full_path[1024];

    project->root_path = strdup(root_path);
    if(jdk_home) {
        project->jdk_root = strdup(jdk_home);
    }
    project->class_file_source = arraylist_new(100);

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
            if(!end_with(entry->d_name, ".class")) {
                continue;
            }
            char *name = strdup(entry->d_name);
            char *dot = strrchr(name, '.');
            if(!dot) {
                free(name);
                continue;
            }
            *dot = '\0';
            class_file_source_t *source = calloc(1, sizeof(class_file_source_t));
            source->name = strdup(name);
            source->source = CLASS_FILE_SOURCE_FILE;
            source->path = strdup(full_path);
            arraylist_add(project->class_file_source, source);
            free(name);
        }

    }
    closedir(dir);

    // load jmod
    char buffer[1024];
    mz_zip_archive *zip = calloc(1, sizeof(mz_zip_archive));

    sprintf(buffer, "%s/jmods/java.base.jmod", project->jdk_root);
    if (mz_zip_reader_init_file(zip, buffer, 0)) {
        project->jmod_base_zip = zip;

        // 获取文件总数
        mz_uint num = mz_zip_reader_get_num_files(zip);
        for (mz_uint i = 0; i < num; i++) {
            mz_zip_archive_file_stat st;

            if (!mz_zip_reader_file_stat(zip, i, &st))
                continue;

            if (st.m_is_directory)
                continue;

            if (!start_with(st.m_filename, "classes/"))
                continue;

            if (!end_with(st.m_filename, ".class"))
                continue;

            char *filename = strdup(st.m_filename);
            char *dot = strrchr(filename, '.');
            if (!dot) {
                free(filename);
                continue;
            }
            *dot = '\0';

            class_file_source_t *cfs = calloc(1, sizeof(class_file_source_t));
            cfs->name = strdup(filename + strlen("classes/"));
            cfs->source = CLASS_FILE_SOURCE_JMOD;
            cfs->index = i;
            free(filename);

            arraylist_add(project->class_file_source, cfs);
        } 
        zip = NULL;
    }else{
        fprintf(stderr, "open jmod failed\n");
        free(zip);
    }

    return project;
}


void project_free(project_t *project) {
    // todo free
}