#include "nova_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_int(int v) {
    printf("%d\n", v);
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
    char* result = malloc(len_a + len_b + 1);
    if (!result) {
        perror("memory allocation failed");
        exit(1);
    }
    strcpy(result, a);
    strcat(result, b);
    return result;
}


char* read_file(const char* path) {
    FILE* file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    if (!file) {
        perror("read_file");
        exit(1);
    }
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (!file) {
        perror("read_file");
        exit(1);
    }
    char* content = malloc(length + 1);
    if (!content) {
        perror("memory allocation failed");
        exit(1);
    }
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    return content;
}


void write_file(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) {
        perror("write_file");
        exit(1);
    }
    fprintf(file, "%s", content);
    fclose(file);
}