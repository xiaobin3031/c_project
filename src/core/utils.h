#pragma once

#include <stdio.h>

char* get_local_ip();

int start_with(const char *str, const char *prefix);

int end_with(const char *str, const char *suffix);

int is_integer(const char *str);

char *fgetline(FILE *fp);