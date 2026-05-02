#pragma once

#include <stdbool.h>


void print_int(int x);

void print_str(const char* s);
bool str_eq(const char* a, const char* b);
char* str_concat(const char* a, const char* b);
int str_len(const char* s);
int str_get(const char* s, int index);
char* str_slice(const char* s, int start, int end);
bool str_starts_with(const char* s, const char* prefix);
bool str_contains(const char* s, const char* needle);
char* int_to_str(int x);

char* read_file(const char* path);
void write_file(const char* path, const char* content);

int buf_new();
void buf_push_str(int buf, const char* s);
void buf_push_int(int buf, int x);
char* buf_to_str(int buf);

int str_vec_new();
void str_vec_push(int vec, const char* s);
char* str_vec_get(int vec, int index);
int str_vec_len(int vec);