#pragma once
#include "../list/arraylist.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct json_object json_object;
typedef struct json_value json_value;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_DOUBLE,
    JSON_INT,
    JSON_BOOL,
    JSON_NULL
} json_type;

typedef enum {
    JSON_TOKEN_STRING,
    JSON_TOKEN_INT,
    JSON_TOKEN_DOUBLE,
    JSON_TOKEN_COLON,   // :
    JSON_TOKEN_COMMA,   // ,
    JSON_TOKEN_LBRACE,  // {
    JSON_TOKEN_RBRACE,  // }
    JSON_TOKEN_LBRACKET,// [
    JSON_TOKEN_RBRACKET,// ]
    JSON_TOKEN_TRUE,    // true
    JSON_TOKEN_FALSE,   // false
    JSON_TOKEN_NULL,    // null
    JSON_TOKEN_EOF
} json_token_type;

struct json_value {
    json_type type;

    union {
        char *value_string;
        double double_number;
        int int_number;
        int value_bool;

        // object or array
        json_object *object;
        arraylist *array;
    };
};

struct json_object {
    arraylist *keys;
    arraylist *values;
};

typedef struct {
    json_token_type type;
    union {
        char ch;
        char *string;
        double double_val;
        int int_val;
    };
} json_token;

json_value *json_parse(char *json_str);
json_value *get_value(const json_value *root, char *xpath);

void json_free(void *value);
void json_object_free(void *object);

void json_print(json_value *value);

void json_token_free(void *token);

void json_token_list_print(arraylist *list);