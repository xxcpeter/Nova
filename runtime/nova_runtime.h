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
bool str_ends_with(const char* s, const char* suffix);
bool str_contains(const char* s, const char* needle);
const char* int_to_str(int x);

const char* read_file(const char* path);
void write_file(const char* path, const char* content);

int buf_new();
void buf_push_str(int buf, const char* s);
void buf_push_int(int buf, int x);
const char* buf_to_str(int buf);

int arg_count();
const char* arg_get(int index);
void nova_runtime_init(int argc, char** argv);

int run_command(const char* command);
bool file_exists(const char* path);
bool dir_exists(const char* path);
void make_dir(const char* path);
void remove_file(const char* path);

void nova_runtime_error(const char* message);