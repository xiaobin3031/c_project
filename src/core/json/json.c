#include "../list/arraylist.h"
#include "../utils.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static size_t g_token_idx = 0;
static arraylist *g_tokens;

static json_token *make_char_token(json_token_type type, char ch) {
    json_token *token = malloc(sizeof(json_token));
    token->type = type;
    token->ch = ch;
    return token;
}

static json_token *make_literal_token(json_token_type type) {
    json_token *token = malloc(sizeof(json_token));
    token->type = type;
    return token;
}

static arraylist *parse_tokens(char *json_str) { 
    char ch;
    arraylist *token_list = arraylist_new(100);

    while((ch = *json_str)) {
        switch(ch) {
            case '{':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_LBRACE, '{'));
                break;
            case '}':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_RBRACE, '}'));
                break;
            case '[':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_LBRACKET, '['));
                break;
            case ']':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_RBRACKET, ']'));
                break;
            case ':':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_COLON, ':'));
                break;
            case ',':
                arraylist_add(token_list, make_char_token(JSON_TOKEN_COMMA, ','));
                break;
            case '"':
                // parse string
                {
                    char *start = json_str + 1;
                    char *end = start;
                    while(*end && *end != '"') end++;
                    size_t len = end - start;
                    char *str = malloc(len + 1);
                    strncpy(str, start, len);
                    str[len] = '\0';
                    json_token *string_token = malloc(sizeof(json_token));
                    string_token->type = JSON_TOKEN_STRING;
                    string_token->string = str;
                    arraylist_add(token_list, string_token);
                    json_str = end; // move to closing "
                }
                break;
            case 't':
                if(strncmp(json_str, "true", 4) == 0) {
                    arraylist_add(token_list, make_literal_token(JSON_TOKEN_TRUE));
                    json_str += 3; // move to 'e'
                }else{
                    perror("json parse error: invalid token[true]");
                    arraylist_free(token_list, json_token_free);
                    return NULL;
                }
                break;
            case 'f':
                if(strncmp(json_str, "false", 5) == 0) {
                    arraylist_add(token_list, make_literal_token(JSON_TOKEN_FALSE));
                    json_str += 4; // move to 'e'
                }else{
                    perror("json parse error: invalid token[false]");
                    arraylist_free(token_list, json_token_free);
                    return NULL;
                }
                break;
            case 'n':
                if(strncmp(json_str, "null", 4) == 0) {
                    arraylist_add(token_list, make_literal_token(JSON_TOKEN_NULL));
                    json_str += 3; // move to 'l'
                }else{
                    perror("json parse error: invalid token[null]");
                    arraylist_free(token_list, json_token_free);
                    return NULL;
                }
                break;
            case ' ': case '\t': case '\n': case '\r':
                // skip whitespace
                break;
            case '\\':
                // skip escape for now
                json_str++;
                break;
            default:
                if(isdigit(ch) || ch == '-' || ch == '.') {
                    char *start = json_str;
                    char *end = start;
                    int hasDot = 0;
                    while(isdigit(*end) || *end == '.') {
                        if(*end == '.') hasDot = 1;
                        end++;
                    }
                    size_t len = end - start;
                    char *str = malloc(len + 1);
                    strncpy(str, start, len);
                    str[len] = '\0';
                    json_token *number_token = malloc(sizeof(json_token));
                    if(hasDot) {
                        number_token->type = JSON_TOKEN_DOUBLE;
                        number_token->double_val = strtod(str, NULL);
                    }else{
                        number_token->type = JSON_TOKEN_INT;
                        number_token->int_val = strtol(str, NULL, 10);
                    }
                    arraylist_add(token_list, number_token);
                    json_str = end - 1;
                }else{
                    printf("invalid char: %c\n", ch);
                    perror("json parse error: invalid token");
                    arraylist_free(token_list, json_token_free);
                    return NULL;
                }
                break;
        }
        json_str++;
    }
    
    json_token *eof_token = malloc(sizeof(json_token));
    eof_token->type = JSON_TOKEN_EOF;
    arraylist_add(token_list, eof_token);
    return token_list;
}

static char *json_token_type_string(json_token_type type) {
    switch(type) {
        case JSON_TOKEN_STRING: return "STRING";
        case JSON_TOKEN_DOUBLE: return "DOUBLE";
        case JSON_TOKEN_INT: return "INT";
        case JSON_TOKEN_COLON: return "COLON";
        case JSON_TOKEN_COMMA: return "COMMA";
        case JSON_TOKEN_LBRACE: return "LBRACE";
        case JSON_TOKEN_RBRACE: return "RBRACE";
        case JSON_TOKEN_LBRACKET: return "LBRACKET";
        case JSON_TOKEN_RBRACKET: return "RBRACKET";
        case JSON_TOKEN_TRUE: return "TRUE";
        case JSON_TOKEN_FALSE: return "FALSE";
        case JSON_TOKEN_NULL: return "NULL";
        case JSON_TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

static int token_end() {
    return g_token_idx >= g_tokens->size || ((json_token*)g_tokens->values[g_token_idx])->type == JSON_TOKEN_EOF;
}

static json_token *peek_token() {
    if(token_end()) return NULL;
    return g_tokens->values[g_token_idx];
}
static json_token *next_token() {
    if(token_end()) return NULL;
    return (json_token*)g_tokens->values[g_token_idx++];
}

static json_value *parse_value() {
    json_token *token = next_token();
    if(token == NULL) return NULL;

    json_value *value;
    switch(token->type) {
        case JSON_TOKEN_STRING:
            value = malloc(sizeof(json_value));
            value->type = JSON_STRING;
            value->value_string = strdup(token->string);
            break;
        case JSON_TOKEN_DOUBLE:
            value = malloc(sizeof(json_value));
            value->type = JSON_DOUBLE;
            value->double_number = token->double_val;
            break;
        case JSON_TOKEN_INT:
            value = malloc(sizeof(json_value));
            value->type = JSON_INT;
            value->int_number = token->int_val;
            break;
        case JSON_TOKEN_TRUE:
            value = malloc(sizeof(json_value));
            value->type = JSON_BOOL;
            value->value_bool = 1;
            break;
        case JSON_TOKEN_FALSE:
            value = malloc(sizeof(json_value));
            value->type = JSON_BOOL;
            value->value_bool = 0;
            break;
        case JSON_TOKEN_NULL:
            value = malloc(sizeof(json_value));
            value->type = JSON_NULL;
            break;
        case JSON_TOKEN_LBRACE:
            // parse object
            json_token *local_token;
            json_object *object = malloc(sizeof(json_object));
            object->keys = arraylist_new(10);
            object->values = arraylist_new(10);

            while(!token_end() && (local_token = next_token())) {
                if(local_token->type == JSON_TOKEN_RBRACE) {
                    break;
                }
                if(local_token->type == JSON_TOKEN_STRING) {
                    if(next_token()->type != JSON_TOKEN_COLON){
                        printf("invalid token, expected colon but got %s\n", json_token_type_string(peek_token()->type));
                        perror("json parse error: invalid token");
                        json_object_free(object);
                        json_free(value);
                        return NULL;
                    }
                    json_value *value = parse_value();
                    arraylist_add(object->keys, strdup(local_token->string));
                    arraylist_add(object->values, value);

                    if(peek_token()->type == JSON_TOKEN_COMMA) next_token();
                }else{
                    printf("invalid token, expected string key but got %s\n", json_token_type_string(local_token->type));
                    perror("json parse error: invalid token");
                    json_object_free(object);
                    json_free(value);
                    return NULL;
                }
            }

            value = malloc(sizeof(json_value));
            value->type = JSON_OBJECT;
            value->object = object;
            break;
        case JSON_TOKEN_LBRACKET:
            // parse array
            {
                json_token *local_token;
                arraylist *array = arraylist_new(10);

                while(!token_end() && (local_token = peek_token())) {
                    if(local_token->type == JSON_TOKEN_RBRACKET) {
                        next_token();
                        break;
                    }
                    json_value *value = parse_value();
                    arraylist_add(array, value);

                    if(peek_token()->type == JSON_TOKEN_COMMA) next_token();
                }

                value = malloc(sizeof(json_value));
                value->type = JSON_ARRAY;
                value->array = array;
            }
            break;
    }
    return value;
}

json_value *json_parse(char *json_str) {
    arraylist *token_list = parse_tokens(json_str);
    if(!token_list) return NULL;

    // json_token_list_print(token_list);

    // parse tokens into json_value
    g_token_idx = 0;
    g_tokens = token_list;
    json_value *root = parse_value();
    if(!token_end()) {
        printf("invalid token, expected EOF but got %s\n", json_token_type_string(peek_token()->type));
        json_free(root);
        root = NULL;
    }

    arraylist_free(token_list, json_token_free);
    return root;
    // return NULL;
}

json_value *get_value(const json_value *root, char *xpath) {
    if(!root || !xpath) return NULL;

    char *path = strdup(xpath);
    char *token = strtok(path, "/");
    const json_value *current = root;

    while(token && current) {
        if(current->type == JSON_OBJECT) {
            json_object *obj = current->object;
            int found = 0;
            for(int i = 0; i < obj->keys->size; i++) {
                char *key = (char*)obj->keys->values[i];
                if(strcmp(key, token) == 0) {
                    current = (json_value*)obj->values->values[i];
                    found = 1;
                    break;
                }
            }
            if(!found) {
                current = NULL;
            }
        } else if(current->type == JSON_ARRAY && is_integer(token)) {
            int index = atoi(token);
            if(index >= 0 && index < current->array->size) {
                current = (json_value*)current->array->values[index];
            } else {
                current = NULL;
            }
        } else {
            current = NULL;
        }
        token = strtok(NULL, "/");
    }

    free(path);
    return (json_value*)current;
}

void json_free(void *val) {
    json_value *value = (json_value*)val;
    if(value) {
        switch(value->type) {
            case JSON_STRING:
                free(value->value_string);
                break;
            case JSON_OBJECT:
                json_object_free(value->object);
                break;
            case JSON_ARRAY:
                // free array values
                for(int i = 0; i < value->array->size; i++) {
                    json_free(value->array->values[i]);
                }
                free(value->array->values);
                free(value->array);
                break;
        }
    }
}

void json_object_free(void *value) {
    json_object *object = (json_object*)value;
    if(object) {
        arraylist_free(object->keys, free);
        arraylist_free(object->values, json_free);
    }
}

void json_print(json_value *value) {
    if(value) {
        switch(value->type) {
            case JSON_STRING:
                printf("\"%s\"", value->value_string);
                break;
            case JSON_DOUBLE:
                printf("%f", value->double_number);
                break;
            case JSON_INT:
                printf("%d", value->int_number);
                break;
            case JSON_BOOL:
                printf("%s", value->value_bool ? "true" : "false");
                break;
            case JSON_NULL:
                printf("null");
                break;
            case JSON_OBJECT:
                printf("{");
                arraylist *keys = value->object->keys;
                for(int i = 0; i < keys->size; i++) {
                    printf("\"%s\": ", keys->values[i]);
                    json_print((json_value*)value->object->values->values[i]);
                    if(i < keys->size - 1) {
                        printf(", ");
                        continue;
                    }
                }
                printf("}");
                break;
            case JSON_ARRAY:
                printf("[");
                for(int i = 0; i < value->array->size; i++) {
                    json_print(value->array->values[i]);
                    if(i < value->array->size - 1) {
                        printf(", ");
                    }
                }
                printf("]");
                break;
        }
    }else{
        printf("json is null");
    }
}

void json_token_free(void *value) {
    json_token *token = (json_token*)value;
    if(token) {
        switch(token->type) {
            case JSON_TOKEN_STRING:
                free(token->string);
                break;
        }
        free(token);
    }
}

void json_token_list_print(arraylist *list) {
    if(list) {
        for(int i=0;i<list->size;i++) {
            json_token *token = (json_token*)arraylist_get(list, i);
            printf("%s: ", json_token_type_string(token->type));
            switch(token->type) {
                case JSON_TOKEN_STRING:
                    printf("%s\n", token->string);
                    break;
                case JSON_TOKEN_TRUE:
                    printf("true\n");
                    break;
                case JSON_TOKEN_FALSE:
                    printf("false\n");
                    break;
                case JSON_TOKEN_NULL:
                    printf("null\n");
                    break;
                case JSON_TOKEN_DOUBLE:
                    printf("%f\n", token->double_val);
                    break;
                case JSON_TOKEN_INT:
                    printf("%d\n", token->int_val);
                    break;
                case JSON_TOKEN_EOF: 
                    printf("\n");
                    break;
                default:
                    printf("%c\n", token->ch);
                    break;
            }
        }
    }
}