#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shared.h"

static int caify_want(FILE* input, FILE* output, const char* objects_dir) {
  uint8_t hash[CAIFY_HASH_SIZE];
  uint32_t count;
  uint32_t one = 1;
  while (true) {
    size_t n = fread(hash, 1, CAIFY_HASH_SIZE, input);
    if (!n && feof(input)) return 0;
    if (n < CAIFY_HASH_SIZE) {
      fprintf(stderr, "Unexpected end of input stream\n");
      return -1;
    }
    n = fread(&count, 1, sizeof(count), input);
    if (n < sizeof(count)) {
      fprintf(stderr, "Unexpected end of input stream\n");
      return -1;
    }
    char dir[PATH_MAX + 1];
    char path[PATH_MAX + 1];
    hash_to_path(objects_dir, hash, dir, path);

    if (access(path, F_OK)) {
      fwrite(hash, CAIFY_HASH_SIZE, 1, output);
      fwrite(&one, sizeof(one), 1, output);
    }

  }
  return 0;
}

int main(int argc, char** argv) {
  return caify_main(argc, argv, caify_want,
    "\033[1;34mfilter\033[0m", "Filter index and emit hashes wanting from local object store.");
}
