/**
 * @file parser.h
 * @brief Shell command line parser for PennOS.
 *
 * This header provides the command line parsing interface for the PennOS
 * shell. It handles tokenization of shell commands including:
 * - Input/output redirection (< > >>)
 * - Pipelines (|)
 * - Background execution (&)
 * - Comments (#)
 *
 * @author hanbangw, 21fa
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Here defines all possible parser errors */

// parser worked
#define PARSER_SUCCESS 0

// parser encountered an unexpected file input token '<'
#define UNEXPECTED_FILE_INPUT 1

// parser encountered an unexpected file output token '>'
#define UNEXPECTED_FILE_OUTPUT 2

// parser encountered an unexpected pipeline token '|'
#define UNEXPECTED_PIPELINE 3

// parser encountered an unexpected ampersand token '&'
#define UNEXPECTED_AMPERSAND 4

// parser didn't find input filename following '<'
#define EXPECT_INPUT_FILENAME 5

// parser didn't find output filename following '>' or '>>'
#define EXPECT_OUTPUT_FILENAME 6

// parser didn't find any commands or arguments where it expects one
#define EXPECT_COMMANDS 7

/**
 * struct parsed_command stored all necessary
 * information needed for penn-shell.
 */
struct parsed_command {
  // indicates the command shall be executed in background
  // (ends with an ampersand '&')
  bool is_background;

  // indicates if the stdout_file shall be opened in append mode
  // ignore this value when stdout_file is NULL
  bool is_file_append;

  // filename for redirecting input from
  const char* stdin_file;

  // filename for redirecting output to
  const char* stdout_file;

  // number of commands (pipeline stages)
  size_t num_commands;

  // an array to a list of arguments
  // size of `commands` is `num_commands`
  char** commands[];
};

/**
 * Arguments:
 *   cmd_line: a null-terminated string that is the command line
 *   result:   a non-null pointer to a `struct parsed_command *`
 *
 * Return value (int):
 *   an error code which can be,
 *       0: parser finished succesfully
 *      -1: parser encountered a system call error
 *     1-7: parser specific error, see error type above
 *
 * This function will parse the given `cmd_line` and store the parsed
 * information into a `struct parsed_command`. The memory needed for the struct
 * will be allocated by this function, and the pointer to the memory will be
 * stored into the given `*result`.
 *
 * You can directly use the result in system calls. See demo for more
 * information.
 *
 * If the function returns a successful value (0), a `struct parsed_command` is
 * guareenteed to be allocated and stored in the given `*result`. It is the
 * caller's responsibility to free the given pointer using `free(3)`.
 *
 * Otherwise, no `struct parsed_command` is allocated and `*result` is
 * unchanged. If a system call error (-1) is returned, the caller can use
 * `errno(3)` or `perror(3)` to gain more information about the error.
 */
int parse_command(const char* cmd_line, struct parsed_command** result);

/**
 * Arguments:
 *   output: where to print parsed command to
 *   cmd: a pointer to a struct parsed_command * with a valid command
 *
 * Print the given command to the provided output location under a standard
 * format. The ampersand (&) character indicating background processes are not
 * included.
 *
 */
void print_parsed_command(int fd, const struct parsed_command* cmd);

/**
 * Arguments:
 *   output: where to print the error to
 *   err_code: the error code to print
 *
 * Prints the error description of err_code to the file stream
 *
 */
void print_parser_errcode(int fd, int err_code);

/**
 * Arguments:
 *   str: the string to parse
 *   out: a pointer to where the result should be assigned if successful.
 *
 * Attempts to parse the string for an id, returning true if successful and
 * false otherwise. If successful, sets *out to the parsed value.
 *
 */
bool parseId(char* str, uint64_t* out);