#pragma once
#include <stdlib.h>

typedef void (*free_sub)(void *);

typedef struct {
    size_t capacity;
    size_t step_capacity;
    size_t size;
    void **values;
} arraylist;

arraylist *arraylist_new(size_t init_capacity);

void arraylist_add(arraylist *list, void *value);
void *arraylist_get(arraylist *list, size_t index);
void *arraylist_set(arraylist *list, size_t index, void *value);
void *arraylist_remove(arraylist *list, size_t index);

void arraylist_clear(arraylist *list, free_sub free_sub);

void arraylist_free(arraylist *list, free_sub free_sub);