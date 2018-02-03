#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "shared.h"
#include "BLAKE2/ref/blake2b-ref.c"

static int save_object(const char* objects_dir, uint8_t* hash, uint8_t* buffer) {
  char dir[PATH_MAX + 1];
  char path[PATH_MAX + 1];
  hash_to_path(objects_dir, hash, dir, path);

  if (mkdir(dir, 0700) && errno != EEXIST) {
    fprintf(stderr, "%s: '%s'\n", strerror(errno), objects_dir);
    return errno;
  }
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (fd < 0) {
    if (errno == EEXIST) return 0;
    fprintf(stderr, "%s: '%s'\n", strerror(errno), path);
    return errno;
  }
  int written = write(fd, buffer, CAIFY_CHUNK_SIZE);
  if (written < 0) {
    fprintf(stderr, "%s: '%s'\n", strerror(errno), path);
    close(fd);
    return errno;
  }
  close(fd);
  return 0;
}

static int record_hash(FILE* output, uint8_t* hash, uint32_t count) {
  if (verbose) {
    for (int i = 0; i < CAIFY_HASH_SIZE; i++) {
      fprintf(stderr, "%02x", hash[i]);
    }
    fprintf(stderr, " %d\n", count);
  }

  fwrite(hash, 1, CAIFY_HASH_SIZE, output);
  fwrite(&count, 1, sizeof(count), output);
  // TODO: add error checking

  return 0;
}

static int caify_import(FILE* input, FILE* output, const char* objects_dir) {

  // Create the object store directory if it doesn't exist already.
  if (mkdir(objects_dir, 0700) && errno != EEXIST) {
    fprintf(stderr, "%s: '%s'\n", strerror(errno), objects_dir);
    return errno;
  }

  uint8_t hash[CAIFY_HASH_SIZE];
  uint8_t new_hash[CAIFY_HASH_SIZE];
  uint32_t count = 0;
  while (true) {
    // Read the next chunk from the file.
    uint8_t buffer[CAIFY_CHUNK_SIZE];
    size_t n = fread(buffer, 1, CAIFY_CHUNK_SIZE, input);

    // Break when the data is done
    if (n < CAIFY_CHUNK_SIZE) break;

    // Hash the chunk
    blake2(new_hash, CAIFY_HASH_SIZE, buffer, CAIFY_CHUNK_SIZE, NULL, 0);

    if (count) {
      // If it matches the last hash, just increment the counter.
      if (memcmp(hash, new_hash, CAIFY_HASH_SIZE) == 0) {
        count++;
        continue;
      }

      int ret = record_hash(output, hash, count);
      if (ret) return ret;
      count = 0;
    }

    // If this is the first of a locally unique hash, try to write it.
    int ret = save_object(objects_dir, new_hash, buffer);
    if (ret) return ret;

    memcpy(hash, new_hash, CAIFY_HASH_SIZE);
    count = 1;
  }

  if (count) {
    record_hash(output, new_hash, count);
  }

  return 0;
}

int main(int argc, char** argv) {
  return caify_main(argc, argv, caify_import,
    "\033[1;32mimport\033[0m", "Import filesystem image into object store and write index file.");
}
