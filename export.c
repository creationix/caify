#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "shared.h"
#include "BLAKE2/ref/blake2b-ref.c"

static int load_object(uint8_t hash[CAIFY_HASH_SIZE], uint8_t buffer[CAIFY_CHUNK_SIZE], const char* objects_dir) {
  char dir[PATH_MAX + 1];
  char path[PATH_MAX + 1];
  hash_to_path(objects_dir, hash, dir, path);

  FILE* stream = fopen(path, "r");
  if (!stream) {
    fprintf(stderr, "%s: '%s'\n", strerror(errno), path);
    return -1;
  }
  size_t n = fread(buffer, 1, CAIFY_CHUNK_SIZE, stream);
  if (n < CAIFY_CHUNK_SIZE) {
    fprintf(stderr, "Problem reading chunk: '%s'\n", path);
    fclose(stream);
    return -1;
  }

  uint8_t* new_hash[CAIFY_HASH_SIZE];
  blake2(new_hash, CAIFY_HASH_SIZE, buffer, CAIFY_CHUNK_SIZE, NULL, 0);
  if (memcmp(new_hash, hash, CAIFY_HASH_SIZE)) {
    fprintf(stderr, "Hash mismatch reading: '%s'\n", path);
    fclose(stream);
    return -1;
  }

  fclose(stream);
  return 0;
}

static int caify_export(FILE* input, FILE* output, const char* objects_dir) {
  uint8_t hash[CAIFY_HASH_SIZE];
  uint32_t count;
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
    uint8_t buffer[CAIFY_CHUNK_SIZE];
    int ret = load_object(hash, buffer, objects_dir);
    if (ret) return ret;
    if (verbose) {
      fprintf(stderr, "Writing object %d\n", count);
    }
    while (count-- > 0) {
      fwrite(buffer, 1, CAIFY_CHUNK_SIZE, output);
    }
  }
}

int main (int argc, char** argv) {
  return caify_main(argc, argv, caify_export,
    "\033[1;31mexport\033[0m", "Export from object store to output file using input index.");
}
