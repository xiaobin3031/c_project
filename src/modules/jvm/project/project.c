#include "project.h"
#include "../../../core/list/arraylist.h"
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

        printf("entry.d_name: %s\n", entry->d_name);
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


void project_free(project_t *project) {
    // todo free
}