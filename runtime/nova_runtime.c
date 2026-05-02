#include "nova_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFERS 1024
#define MAX_STR_VECS 1024


static int buffer_count = 0;
static char* buffers[MAX_BUFFERS];
static int str_vec_count = 0;
static char** str_vecs[MAX_STR_VECS];
static int str_vec_lengths[MAX_STR_VECS];


static void runtime_error(const char* message) {
    fprintf(stderr, "Nova runtime error: %s\n", message);
    exit(1);
}


static void* rt_malloc(size_t size) {
    if (size == 0) size = 1;
    void* ptr = malloc(size);
    if (!ptr) {
        runtime_error("memory allocation failed");
    }
    return ptr;
}


static void* rt_realloc(void* ptr, size_t size) {
    if (size == 0) size = 1;
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        runtime_error("memory allocation failed");
    }
    return new_ptr;
}


static char* rt_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = rt_malloc(len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}
    
    
void print_int(int x) {
    printf("%d\n", x);
}


void print_str(const char* s) {
    printf("%s\n", s);
}


bool str_eq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}


char* str_concat(const char* a, const char* b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = rt_malloc(len_a + len_b + 1);
    strcpy(result, a);
    strcat(result, b);
    return result;
}


int str_len(const char* s) {
    return strlen(s);
}


int str_get(const char* s, int index) {
    if (index < 0 || index >= strlen(s)) {
        runtime_error("string index out of bounds");
    }
    return s[index];
}


char* str_slice(const char* s, int start, int end) {
    if (start < 0 || end > strlen(s) || start > end) {
        runtime_error("invalid string slice indices");
    }
    size_t len = end - start;
    char* result = rt_malloc(len + 1);
    strncpy(result, s + start, len);
    result[len] = '\0';
    return result;
}


bool str_starts_with(const char* s, const char* prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}


bool str_contains(const char* s, const char* needle) {
    return strstr(s, needle) != NULL;
}


char* int_to_str(int x) {
    char* result = rt_malloc(12); // Enough for a 32-bit integer
    snprintf(result, 12, "%d", x);
    return result;
}


char* read_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        runtime_error("failed to open file");
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        runtime_error("failed to seek file");
    }
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* content = rt_malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    return content;
}


void write_file(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) {
        runtime_error("failed to open file");
    }
    fprintf(file, "%s", content);
    fclose(file);
}


int buf_new() {
    if (buffer_count >= MAX_BUFFERS) {
        runtime_error("too many buffers");
    }
    buffers[buffer_count] = rt_malloc(1);
    buffers[buffer_count][0] = '\0';
    buffer_count++;
    return buffer_count - 1;
}


void buf_push_str(int buf, const char* s) {
    buffers[buf] = rt_realloc(buffers[buf], strlen(buffers[buf]) + strlen(s) + 1);
    strcat(buffers[buf], s);
}


void buf_push_int(int buf, int x) {
    char* s = int_to_str(x);
    buf_push_str(buf, s);
    free(s);
}


char* buf_to_str(int buf) {
    return buffers[buf];
}


int str_vec_new() {
    if (str_vec_count >= MAX_STR_VECS) {
        runtime_error("too many string vectors");
    }
    str_vecs[str_vec_count] = rt_malloc(sizeof(char*));
    str_vec_count++;
    return str_vec_count - 1;
}


void str_vec_push(int vec, const char* s) {
    int len = str_vec_len(vec);
    str_vecs[vec] = rt_realloc(str_vecs[vec], (len + 1) * sizeof(char*));
    str_vecs[vec][len] = rt_strdup(s);
    str_vec_lengths[vec] = len + 1;
}


char* str_vec_get(int vec, int index) {
    if (index < 0 || index >= str_vec_len(vec)) {
        runtime_error("string vector index out of bounds");
    }
    return str_vecs[vec][index];
}


int str_vec_len(int vec) {
    return str_vec_lengths[vec];
}