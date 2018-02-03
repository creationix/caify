#ifndef CAIFY_SHARED_C
#define CAIFY_SHARED_C

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "BLAKE2/ref/blake2b-ref.c"

#define CAIFY_HASH_SIZE 32
#define CAIFY_CHUNK_SIZE 4096
#define PATH_MAX 1024

#define toNibble(b) ((b) < 10 ? '0' + (b) : 'a' - 10 + (b))

static void hash_to_path(const char* objects_dir, uint8_t* hash, char* dir, char* path) {
  snprintf(dir, PATH_MAX, "%s/%02x/", objects_dir, hash[0]);

  strncpy(path, dir, PATH_MAX);
  snprintf(path, PATH_MAX, "%s/", dir);
  int offset = strlen(dir);
  for (int i = 1; i < CAIFY_HASH_SIZE; i++) {
    if (offset >= PATH_MAX) break;
    path[offset++] = toNibble(hash[i] >> 4);
    if (offset >= PATH_MAX) break;
    path[offset++] = toNibble(hash[i] & 0xf);
  }
  path[offset++] = 0;

}

#endif
