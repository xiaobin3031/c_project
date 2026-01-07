#pragma once

#include "../utils/bytes.h"
#include "../classfile/class_reader.h"
#include <pthread.h>

typedef struct class_t class_t;
typedef struct slot_t slot_t;
typedef struct object_t object_t;

enum class_state {
    CLASS_UNLOADED = 0,
    CLASS_LOADED = 1,
    CLASS_LINKED = 2,
    CLASS_INITING = 3,
    CLASS_INITIALIZED = 4,
    CLASS_ERRONEOUS = 5
};

typedef enum {
    RT_CONSTANT_Utf8 = 1,

    RT_CONSTANT_Integer = 3,
    RT_CONSTANT_Float = 4,
    RT_CONSTANT_Long = 5,
    RT_CONSTANT_Double = 6,
    RT_CONSTANT_Class = 7,
    RT_CONSTANT_String = 8,
    RT_CONSTANT_Fieldref = 9,
    RT_CONSTANT_Methodref = 10,
    RT_CONSTANT_InterfaceMethodref = 11,
    RT_CONSTANT_NameAndType = 12,

    RT_CONSTANT_MethodHandle = 15,
    RT_CONSTANT_MethodType = 16,
    RT_CONSTANT_Dynamic = 17,
    RT_CONSTANT_InvokeDynamic = 18,
    RT_CONSTANT_Module = 19,
    RT_CONSTANT_Package = 20
} rt_cp_tag;

typedef struct {
    char *utf8;
    size_t length;
} native_string_t;

typedef struct {
    u1 *codes;
    u4 code_length;

    u2 max_stack;
    u2 max_locals;
} rt_code_t;

typedef struct {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
} rt_exception_table_t;

typedef struct {
    u2 access_flags;
    char *name;
    char *descriptor;

    rt_code_t *code;
    u2 exception_table_length;
    rt_exception_table_t *exception_table;

        // 方法入参个数，这两个参数从descriptor中解析,  long/double占两个slot
    u2 arg_slot_count;
    // 返回值个数
    u2 return_slot_count;
} method_t;

typedef struct {
    u2 access_flags;
    char *name;
    char *descriptor;

    u2 slot_id;
    u2 slot_count;
    slot_t *slots;
} field_t;

typedef struct {
    u1 tag;
    u1 resolved;

    union {

        struct {
            char *class_name;
            char *name;
            char *descriptor;
        } sym;

        class_t *klass;
        field_t *field;
        method_t *method;
        native_string_t *strings;

        struct {
            int32_t int_value;
            long long_value;
            float float_value;
            double double_value;
        } number;
    };

} rt_cp_entry_t;

struct class_t {
    int is_array;

    u2 access_flags;

    char *class_name;

    char *class_simple_name;

    // 属性的slot总数
    u2 total_field_slots;

    pthread_mutex_t lock;
    enum class_state state;

    class_t *super;

    class_t **interface_class;

    u2 methods_count;
    method_t *methods;

    u2 fields_count;
    field_t *fields;

    u2 rt_cp_count;
    rt_cp_entry_t *entries;

    char *super_class_name;

    u2 interface_count;
    char **interface_class_names;
};

typedef enum {

    OBJ_TYPE_INSTANCE,
    OBJ_TYPE_ARRAY,

} object_type_e;

struct object_t {
    object_type_e type;
    class_t *klass;

    union {
        struct {
            slot_t *fields;
        } instance;

        struct {
            int length;
            slot_t *elements;
        } array;
    };

    void *native_string;
};

struct slot_t {
    uint32_t bits;
    object_t *ref;
};

class_t *define_class(class_file_t *class_file);

native_string_t *string_new(const char *utf8);