#include "arraylist.h"
#include <stdlib.h>

arraylist *arraylist_new(size_t init_capacity) {
    arraylist *list = malloc(sizeof(arraylist));
    list->capacity = init_capacity;
    list->step_capacity = init_capacity;
    list->size = 0;
    list->values = malloc(sizeof(void*) * list->capacity);
    return list;
}

void arraylist_add(arraylist *list, void *value) { 
    if (list->size == list->capacity) {
        list->capacity = list->step_capacity * 2;
        list->values = realloc(list->values, sizeof(void*) * list->capacity);
    }
    list->values[list->size++] = value;
}

void *arraylist_get(arraylist *list, size_t index) {
    if(index >= list->size) return NULL;

    return list->values[index];
}
void *arraylist_set(arraylist *list, size_t index, void *value) {
    if(index >= list->size) return NULL;

    void *old_value = list->values[index];
    list->values[index] = value;
    return old_value;
}

void *arraylist_remove(arraylist *list, size_t index) {
    if(index >= list->size) return NULL;

    void *removed_value = list->values[index];
    for(size_t i = index; i < list->size - 1; i++) {
        list->values[i] = list->values[i + 1];
    }
    list->size--;
    return removed_value;
}

void arraylist_clear(arraylist *list, free_sub free_sub) {
    if(free_sub) {
        for(size_t i = 0; i < list->size; i++) {
            free_sub(list->values[i]);
        }
    }
    list->size = 0;
    list->capacity = list->step_capacity;
    free(list->values);
    list->values = malloc(sizeof(void*) * list->capacity);
}

void arraylist_free(arraylist *list, free_sub free_sub) {
    if(list) {
        if(free_sub) {
            for(size_t i = 0; i < list->size; i++) {
                if(list->values[i]) free_sub(list->values[i]);
            }
        }
        free(list->values);
        free(list);
    }
}