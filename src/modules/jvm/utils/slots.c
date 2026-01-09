#include "slots.h"
#include "bytes.h"
#include <string.h>

u2 slot_count_from_desciptor(char *descriptor) {
    char *ptr = descriptor;
    if(*ptr == '(') ptr++;
    u2 arg_count = 0;
    while(*ptr && *ptr != ')') {
        if(*ptr == '[') {
            // 数组，不影响计数
            ptr++;
            while(*ptr == '[') ptr++;
            if(*ptr == 'L') {
                char *start = ptr+1;
                char *end = start;
                while(*end != ';') end++;
                ptr = end;
            }
            ptr++;
            arg_count++;
            continue;
        }

        if(*ptr == 'L') {
            char *start = ptr+1;
            char *end = start;
            while(*end != ';') end++;
            ptr = end + 1;
            arg_count++;
        }else if(*ptr == 'J' || *ptr == 'D') {
            arg_count+=2;
            ptr++;
        }else{
            if(*ptr != 'V') arg_count++;
            ptr++;
        }
    }
    return arg_count;
}

char *descriptor_to_simple_type(const char *descriptor) {
    const char *start = strrchr(descriptor, '/');
    const char *end = strrchr(descriptor, ';');
    return strndup(start + 1, end - start - 1);
}

char *descriptor_to_type(const char *descriptor) {
    char *desc = strdup(descriptor);
    if(*desc == 'L') {
        desc++;
    }
    char *ptr = desc;
    while(*ptr && *ptr != ';') {
        if(*ptr == '/') {
            *ptr = '.';
        }
        ptr++;
    }
    *ptr = '\0';
    return desc;
}