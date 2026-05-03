#pragma once

#include <stdbool.h>


void print_int(int x);

void print_str(const char* s);
bool str_eq(const char* a, const char* b);
const char* str_concat(const char* a, const char* b);
int str_len(const char* s);
int str_get(const char* s, int index);
const char* str_slice(const char* s, int start, int end);
bool str_starts_with(const char* s, const char* prefix);
bool str_contains(const char* s, const char* needle);
const char* int_to_str(int x);

const char* read_file(const char* path);
void write_file(const char* path, const char* content);

int buf_new();
void buf_push_str(int buf, const char* s);
void buf_push_int(int buf, int x);
const char* buf_to_str(int buf);

int str_vec_new();
void str_vec_push(int vec, const char* s);
const char* str_vec_get(int vec, int index);
int str_vec_len(int vec);

int arg_count();
const char* arg_get(int index);
void nova_runtime_init(int argc, char** argv);

void exit_with_error(const char* message);