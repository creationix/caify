#ifndef CAIFY_SHARED_H
#define CAIFY_SHARED_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define CAIFY_HASH_SIZE 32
#define CAIFY_CHUNK_SIZE 4096
#define PATH_MAX 1024

bool quiet;
bool verbose;

void hash_to_path(const char* objects_dir, uint8_t* hash, char* dir, char* path);

typedef int (caify_fn_t)(FILE* input, FILE* output, const char* objects_dir);
int caify_main(int argc, char** argv, caify_fn_t* action, const char* description);

#endif
