#include "shared.c"

int caify_export(FILE* input, FILE* output, const char* objects_dir);

static bool quiet = false;
static bool verbose = false;

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
        if (!quiet) fprintf(stderr, "Reading from '%s'...\n", optarg);
        input = fopen(optarg, "r");
        break;
      case 'o':
        if (!quiet) fprintf(stderr, "Exporting to '%s'...\n", optarg);
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
      if (!quiet) fprintf(stderr, "Reading from '%s'...\n", argv[i]);
    } else if (!output) {
      output = fopen(argv[i], "w");
      if (!quiet) fprintf(stderr, "Exporting to '%s'...\n", argv[i]);
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
      if (!quiet) fprintf(stderr, "Reading from stdin pipe...\n");
      input = stdin;
    }
  }
  if (!output) {
    if (isatty(1)) {
      fprintf(stderr, "Cannot write to stdout TTY\n");
      usage = true;
    } else {
      if (!quiet) fprintf(stderr, "Exporting to stdout pipe...\n");
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
  if (!quiet) fprintf(stderr, "Exporting from '%s'...\n", objects_dir);

  int ret = caify_export(input, output, objects_dir);

  if (output != stdout) fclose(output);
  if (input != stdin) fclose(input);

  return ret;

}

static int load_object(const char* objects_dir, uint8_t* hash, uint8_t* buffer) {
  char dir[PATH_MAX + 1];
  char path[PATH_MAX + 1];
  hash_to_path(objects_dir, hash, dir, path);

  FILE* stream = fopen(path, "r");
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

  return 0;
}

int caify_export(FILE* input, FILE* output, const char* objects_dir) {
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
    int ret = load_object(objects_dir, hash, buffer);
    if (ret) return ret;
    if (verbose) {
      fprintf(stderr, "Writing object %d\n", count);
    }
    while (count-- > 0) {
      fwrite(buffer, 1, CAIFY_CHUNK_SIZE, output);
    }
  }
}
