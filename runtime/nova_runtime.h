#pragma once

#include <stdbool.h>


void print_int(int v);
void print_str(const char* s);
bool str_eq(const char* a, const char* b);
char* str_concat(const char* a, const char* b);
char* read_file(const char* path);
void write_file(const char* path, const char* content);