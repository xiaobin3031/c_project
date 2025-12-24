#include "slots.h"
#include "bytes.h"

u2 slot_count_from_desciptor(char *descriptor) {
    char *ptr = descriptor + 1;
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
            ptr++;
            arg_count++;
        }
    }
    return arg_count;
}