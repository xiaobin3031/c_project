#include "jmemory.h"
#include "class.h"
#include <stdio.h>
#include <pthread.h>

static jvm_heap_t g_heap;

static void add_heap(object_t *obj) {
    size_t index;
    pthread_mutex_lock(&g_heap.lock);
    if(g_heap.size >= g_heap.capacity) {
        g_heap.capacity += HEAP_GROW_LENGTH;
        g_heap.objects = realloc(g_heap.objects, g_heap.capacity * sizeof(object_t *));
    }
    index = g_heap.size++;
    pthread_mutex_unlock(&g_heap.lock);
    g_heap.objects[index] = obj;
}

void init_heap() {
    g_heap.capacity = HEAP_GROW_LENGTH;
    g_heap.size = 0;
    g_heap.objects = calloc(g_heap.capacity, sizeof(object_t *));

    pthread_mutex_init(&g_heap.lock, NULL);
}


object_t *heap_alloc_object(class_t *klass) {
    object_t *obj = calloc(1, sizeof(object_t));
    obj->klass = klass;
    obj->type = OBJ_TYPE_INSTANCE;
    obj->instance.fields = calloc(klass->total_field_slots, sizeof(slot_t));
    add_heap(obj);
    return obj;
}

object_t *heap_alloc_array(class_t *klass, int length) {
    object_t *obj = calloc(1, sizeof(object_t));
    obj->klass = klass;
    obj->type = OBJ_TYPE_ARRAY;
    obj->array.elements = calloc(length, sizeof(slot_t));
    obj->array.length = length;
    add_heap(obj);
    return obj;
}