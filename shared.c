#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shared.h"

#define toNibble(b) ((b) < 10 ? '0' + (b) : 'a' - 10 + (b))

void hash_to_path(const char* objects_dir, uint8_t* hash, char* dir, char* path) {
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


int caify_main(int argc, char** argv, caify_fn_t* action, const char* name, const char* description) {
  FILE* output = NULL;
  FILE* input = NULL;
  char input_path[PATH_MAX + 1];
  char output_path[PATH_MAX + 1];
  char objects_dir[PATH_MAX + 1];
  objects_dir[0] = 0;
  int c;
  bool usage = false;
  while ((c = getopt (argc, argv, "hqvs:i:o:")) != -1) {
    switch (c) {
      case 'h':
        usage = true;
        return -1;
      case 'v':
        verbose = true;
        break;
      case 'q':
        quiet = true;
        break;
      case 's':
        strncpy(objects_dir, optarg, PATH_MAX);
        break;
      case 'i':
        snprintf(input_path, PATH_MAX, "'%s'", optarg);
        input = fopen(optarg, "r");
        if (!input) {
          fprintf(stderr, "%s: '%s'\n", strerror(errno), optarg);
          return -1;
        }
        break;
      case 'o':
        snprintf(output_path, PATH_MAX, "'%s'", optarg);
        output = fopen(optarg, "w");
        if (!output) {
          fprintf(stderr, "%s: '%s'\n", strerror(errno), optarg);
          return -1;
        }
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
      if (!input) {
        fprintf(stderr, "%s: '%s'\n", strerror(errno), argv[i]);
        return -1;
      }
      snprintf(input_path, PATH_MAX, "'%s'", argv[i]);
    } else if (!output) {
      output = fopen(argv[i], "w");
      if (!output) {
        fprintf(stderr, "%s: '%s'\n", strerror(errno), argv[i]);
        return -1;
      }
      snprintf(output_path, PATH_MAX, "'%s'", argv[i]);
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
      fprintf(stderr, "Refusing to read from stdin TTY\n");
    } else {
      snprintf(input_path, PATH_MAX, "(stdin)");
      input = stdin;
    }
  }
  if (!output) {
    if (isatty(1)) {
      fprintf(stderr, "Refusing to write to stdout TTY\n");
      usage = true;
    } else {
      snprintf(output_path, PATH_MAX, "(stdout)");
      output = stdout;
    }
  }

  if (usage) {
    fprintf(stderr, "Caify - %s\n", description);
    fprintf(stderr, "Usage: %s\n  -s path/to/objects\n  -i input\n  -o output\n  -h\n  -v\n", argv[0]);
    return -1;
  }
  if (!quiet) {
    char hostname[PATH_MAX + 1];
    gethostname(hostname, PATH_MAX);
    char message[PATH_MAX + 1];
    if (*objects_dir) {
      snprintf(message, PATH_MAX, "\033[0;37m%s\033[0m: Caify %s from \033[1;35m%s\033[0m to \033[1;36m%s\033[0m using '\033[1;33m%s\033[0m'.\n",
        hostname, name, input_path, output_path, objects_dir);
    } else {
      snprintf(message, PATH_MAX, "\033[0;37m%s\033[0m: Caify %s from \033[1;35m%s\033[0m to \033[1;36m%s\033[0m.\n",
        hostname, name, input_path, output_path);
    }
    fprintf(stderr, "%s", message);
  }

  if (!*objects_dir) {
    snprintf(objects_dir, PATH_MAX, "%s/.caify-objects", getenv("HOME"));
  }
  if (verbose) fprintf(stderr, "Objects: '%s'\n", objects_dir);

  int ret = action(input, output, objects_dir);

  if (output != stdout) fclose(output);
  if (input != stdin) fclose(input);

  return ret;

}
