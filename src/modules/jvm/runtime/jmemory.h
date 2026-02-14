#pragma once

#include "class.h"
#include <stdio.h>
#include <pthread.h>

#define HEAP_GROW_LENGTH 1024

typedef struct {

    object_t **objects;
    size_t size;
    size_t capacity;

    pthread_mutex_t lock;

} jvm_heap_t;

void init_heap();


object_t *heap_alloc_object(class_t *klass);
object_t *heap_alloc_object_by_type(object_type_e type);

object_t *heap_alloc_array(class_t *klass, int length);