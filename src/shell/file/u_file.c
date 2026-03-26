/**
 * @file file.c
 * @brief Implementation of user-level file commands for the PennOS shell.
 *
 * This file implements the built-in shell commands for file manipulation,
 * including cat, ls, touch, mv, cp, rm, and chmod. These commands operate
 * on the PennFAT filesystem using the s_* system call interface.
 */

#include "u_file.h"

#include "io/path/s_path.h"
#include "io/term/s_term.h"

#include <string.h>
#include "util/vector.h"

/**
 * @brief Concatenate and display file contents.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_cat(void* arg) {
  char** argv = (char**)arg;

  char buffer[4096];
  int outfd = STDOUT_FILENO;

  /* Collect all target files before encountering mode flags*/
  vector(char*) targets = vector_new(char*, 1, NULL);
  for (int i = 1; argv[i] != NULL; i++) {
    char* targ = argv[i];
    if (strcmp(targ, "-w") == 0 || strcmp(targ, "-a") == 0) {
      char* mode_arg = targ;
      char* outname = argv[i + 1];
      if (outname == NULL) {
        s_perror("cat: missing output file for %s\n", mode_arg);
        vector_free(&targets);
        return NULL;
      }

      /* Validate input files eexist before opening output file,
       * preventing partial writes into output */
      for (int i = 0; i < vector_len(&targets); i++) {
        int infd = s_open(targets[i], F_READ);
        if (infd < 0) {
          vector_free(&targets);
          return NULL;
        }
        s_close(infd);
      }

      /* Append mode preserves existing content, write mode truncates*/
      int mode = (strcmp(mode_arg, "-a") == 0) ? F_APPEND : F_WRITE;
      outfd = s_open(outname, mode);
      if (outfd < 0) {
        vector_free(&targets);
        return NULL;
      }
      break;
    } else {
      vector_push(&targets, targ);
    }
  }

  if (vector_len(&targets)) {
    /* Validate no input or output overlap to prevent infinite loops when
     * reading from and appending to the same file*/
    for (int i = 0; i < vector_len(&targets); i++) {
      int infd = s_open(targets[i], F_READ);
      if (infd < 0) {
        vector_free(&targets);
        return NULL;
      }
      if (s_same_source(infd, outfd) && s_get_open_mode(outfd) == F_APPEND) {
        s_perror("cat: input and output cannot be the same when appending\n");
        vector_free(&targets);
        return NULL;
      }
      s_close(infd);
    }
    /* Process each input file sequentially and concat the contents*/
    for (int i = 0; i < vector_len(&targets); i++) {
      int infd = s_open(targets[i], F_READ);
      if (infd < 0) {
        vector_free(&targets);
        return NULL;
      }
      size_t n;
      while ((n = s_read(infd, buffer, sizeof(buffer))) > 0) {
        if (s_write(outfd, buffer, (int)n) < 0) {
          s_perror("cat: error writing to output file\n");
          vector_free(&targets);
          return NULL;
        }
      }
      s_close(infd);
    }
  } else {
    /* No files specified meaning read from STDIN*/
    if (s_same_source(STDIN_FILENO, outfd) &&
        s_get_open_mode(outfd) == F_APPEND) {
      s_perror("cat: input and output cannot be the same when appending\n");
      vector_free(&targets);
      return NULL;
    }

    /* Stream STDIN to output until EOF signal*/
    int n;
    while ((n = s_read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
      if (s_write(outfd, buffer, n) < 0) {
        s_perror("cat: error writing to output file\n");
        vector_free(&targets);
        return NULL;
      }
    }
    s_close(outfd);
  }
  vector_free(&targets);
  return NULL;
}

/**
 * @brief List directory contents.
 *
 * Calls s_ls to display file information.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_ls(void* arg) {
  char** argv = (char**)arg;
  const char* filename = (argv[1] != NULL) ? argv[1] : NULL;
  if (s_ls(filename) == -1) {
    s_perror("ls failed\n");
  }
  return NULL;
}

/**
 * @brief Create files or update timestamps.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_touch(void* arg) {
  char** argv = (char**)arg;
  (void)argv;

  if (argv[1] == NULL)
    return NULL;

  /* Opening in append mode creates file if it does not exist and closing adds
   * the timestamp*/
  for (int i = 1; argv[i] != NULL; i++) {
    int fd = s_open(argv[i], F_APPEND);
    if (fd < 0) {
      return NULL;
    }
    s_close(fd);
  }

  return NULL;
}

/**
 * @brief Move or rename a file.
 *
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_mv(void* arg) {
  char** argv = (char**)arg;

  /* Require both source and destination arguments*/
  if (argv[1] == NULL || argv[2] == NULL) {
    s_perror("mv: missing file operand\n");
    s_perror("Usage: mv SOURCE DEST\n");
    return NULL;
  }

  const char* source = argv[1];
  const char* dest = argv[2];

  /* Use s_rename_file to perform the move/rename */
  if (s_mv(source, dest) < 0) {
    s_perror("mv: cannot rename %s to %s\n", source, dest);
    return NULL;
  }

  return NULL;
}

/**
 * @brief Copy a file with optional host filesystem support.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_cp(void* arg) {
  char** argv = (char**)arg;

  /* Parse arguments for -h flag */
  char *source, *dest;
  int sfd = -1, dfd = -1;

  /* cp [-h] SOURCE DEST  or  cp SOURCE -h DEST */
  int p = 1;
  if (argv[p] == NULL) {
    s_perror("cp: missing file operand\n");
    s_perror("Usage: cp [-h] SOURCE [-h] DEST\n");
  }
  if (strcmp(argv[p], "-h") == 0) {
    /* Host filesystem source requires different open function*/
    p++;
    if (argv[p] == NULL) {
      s_perror("cp: missing file operand\n");
      s_perror("Usage: cp [-h] SOURCE [-h] DEST\n");
    }
    source = argv[p++];
    sfd = s_ext_open(source, F_READ);
    if (sfd < 0) {
      s_perror("%s: cannot open the file\n", source);
      return NULL;
    }
  } else {
    /* Default source is PennFAT filesystem*/
    source = argv[p++];
    sfd = s_open(source, F_READ);
    if (sfd < 0) {
      return NULL;
    }
  }

  if (argv[p] == NULL) {
    s_perror("cp: missing file operand\n");
    s_perror("Usage: cp [-h] SOURCE [-h] DEST\n");
  }
  if (strcmp(argv[p], "-h") == 0) {
    /* Host filesystem destination requires different open function */
    p++;
    if (argv[p] == NULL) {
      s_perror("cp: missing file operand\n");
      s_perror("Usage: cp [-h] SOURCE [-h] DEST\n");
    }
    dest = argv[p++];
    dfd = s_ext_open(dest, F_WRITE);
    if (dfd < 0) {
      s_perror("%s: cannot create the file\n", dest);
      return NULL;
    }
  } else {
    /* Default destination is PennFAT filesystem */
    dest = argv[p++];
    dfd = s_open(dest, F_WRITE);
    if (dfd < 0) {
      return NULL;
    }
  }

  /* Copy in chunks for memory efficiency, especially with large files */
  char buffer[4096];
  int bytes_read;

  while ((bytes_read = s_read(sfd, buffer, sizeof(buffer))) > 0) {
    if (s_write(dfd, buffer, bytes_read) < 0) {
      s_perror("cp: error writing to '%s'\n", dest);
      return NULL;
    }
  }
  s_close(sfd);
  s_close(dfd);
  return NULL;
}

/**
 * @brief Remove files from the filesystem.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_rm(void* arg) {
  char** argv = (char**)arg;
  /* Require at least one file to remove */
  if (argv[1] == NULL) {
    s_perror("rm: missing operand\n");
    s_perror("Usage: rm FILE...\n");
    return NULL;
  }
  /* Process all files even if some fail */
  for (int i = 1; argv[i] != NULL; i++) {
    if (s_unlink(argv[i]) < 0) {
      s_perror("rm: cannot remove %s\n", argv[i]);
      continue;
    }
  }
  return NULL;
}

/**
 * @brief Change file permissions.
 *
 * @param arg Pointer to NULL-terminated argv array.
 * @return Always NULL.
 */
void* u_chmod(void* arg) {
  char** argv = (char**)arg;
  (void)argv;

  if (argv[1] == NULL || argv[2] == NULL) {
    s_perror("chmod is missing it's operand\n");
    return NULL;
  }

  char* permission_arg = argv[1];
  char* filename = argv[2];

  /* First character determines whether adding or removing permissions */
  int add = 0;
  if (permission_arg[0] == '+') {
    add = 1;
  } else if (permission_arg[0] == '-') {
    add = 0;
  } else {
    s_perror("chmod choose + or -\n");
    return NULL;
  }

  /* Build permission bitmask from character flags */
  unsigned char flag = 0;
  for (int i = 1; permission_arg[i] != '\0'; i++) {
    if (permission_arg[i] == 'r') {
      flag |= 4;
    } else if (permission_arg[i] == 'w') {
      flag |= 2;
    } else if (permission_arg[i] == 'x') {
      flag |= 1;
    } else {
      s_perror("chmod invalid permission character\n");
      return NULL;
    }
  }

  /* Apply the permission change via system call */
  if (s_chmod(filename, flag, add) < 0) {
    s_perror("chmod no such file\n");
    return NULL;
  }
  return NULL;
}