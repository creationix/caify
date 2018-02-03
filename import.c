#include "shared.c"

// Main import function can also be used as library
int caify_import(FILE* input, FILE* output, const char* objects_dir);

static int record_hash(FILE* output, uint8_t* hash, uint32_t count);
static int save_object(const char* objects_dir, uint8_t* hash, uint8_t* buffer);

static bool quiet = false;
static bool verbose = false;

// import object-store input-file output-file
int main(int argc, char** argv) {
  FILE* output = NULL;
  FILE* input = NULL;
  char objects_dir[PATH_MAX + 1];
  objects_dir[0] = 0;
  int c;
  bool usage = false;
  while ((c = getopt (argc, argv, "hvqs:i:o:")) != -1) {
    switch (c) {
      case 'h':
        fprintf(stderr, "Caify - Import filesystem image to object store and write index file\n\n");
        usage = true;
        return -1;
      case 'q':
        quiet = true;
        break;
      case 'v':
        verbose = true;
        break;
      case 's':
        strncpy(objects_dir, optarg, PATH_MAX);
        break;
      case 'i':
        if (!quiet) fprintf(stderr, "Importing from '%s'...\n", optarg);
        input = fopen(optarg, "r");
        break;
      case 'o':
        if (!quiet) fprintf(stderr, "Writing to '%s'...\n", optarg);
        output = fopen(optarg, "w");
        break;
      case '?':
        return 1;
      default:
        abort ();
    }
  }

  for (int i = optind; i < argc; i++) {
    if (!input) {
      input = fopen(argv[i], "r");
      if (!quiet) fprintf(stderr, "Importing from '%s'...\n", argv[i]);
    } else if (!output) {
      output = fopen(argv[i], "w");
      if (!quiet) fprintf(stderr, "Writing to '%s'...\n", argv[i]);
    } else if (!*objects_dir) {
      strncpy(objects_dir, argv[i], PATH_MAX);
    } else {
      fprintf(stderr, "Unexpected argument: '%s'\n", argv[i]);
      usage = true;
    }
  }

  if (!input) {
    if (isatty(0)) {
      usage = true;
      fprintf(stderr, "Cannot import from stdin TTY\n");
    } else {
      if (!quiet) fprintf(stderr, "Importing from stdin pipe...\n");
      input = stdin;
    }
  }
  if (!output) {
    if (isatty(1)) {
      fprintf(stderr, "Cannot write to stdout TTY\n");
      usage = true;
    } else {
      if (!quiet) fprintf(stderr, "Writing to stdout pipe...\n");
      output = stdout;
    }
  }

  if (usage) {
    fprintf(stderr, "Usage: %s\n  -s path/to/objects\n  -i input.img\n  -o output.idx\n  -h\n  -q\n  -v\n", argv[0]);
    return -1;
  }

  if (!*objects_dir) {
    snprintf(objects_dir, PATH_MAX, "%s/.caify-objects", getenv("HOME"));
  }
  if (!quiet) fprintf(stderr, "Importing into '%s'...\n", objects_dir);

  int ret = caify_import(input, output, objects_dir);

  if (output != stdout) fclose(output);
  if (input != stdin) fclose(input);

  return ret;
}

int caify_import(FILE* input, FILE* output, const char* objects_dir) {

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

static int record_hash(FILE* output, uint8_t* hash, uint32_t count) {
  if (verbose) {
    for (int i = 0; i < CAIFY_HASH_SIZE; i++) {
      fprintf(stdout, "%02x", hash[i]);
    }
    fprintf(stdout, " %d\n", count);
  }

  fwrite(hash, 1, CAIFY_HASH_SIZE, output);
  fwrite(&count, 1, sizeof(count), output);
  // TODO: add error checking

  return 0;
}

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
    return errno;
  }
  close(fd);
  return 0;
}
