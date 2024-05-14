/* fth.h -- https://github.com/takeiteasy/fth
 
 The MIT License (MIT)
 
 Copyright (c) 2024 George Watson
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef FTH_HEADER
#define FTH_HEADER
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <io.h>
#include <dirent.h>
#define F_OK 0
#define access _access
#define EXPORT __declspec(dllexport)
#else
#include <unistd.h>
#define EXPORT
#endif

#if !defined(FTH_DEFAULT_STACK_SIZE)
#define FTH_DEFAULT_STACK_SIZE 32
#endif

#if !defined(FTH_DEFAULT_MAX_WORD_SIZE)
#define FTH_DEFAULT_MAX_WORD_SIZE 256
#endif

typedef unsigned long fth_int_t;
typedef double fth_float_t;

typedef enum {
    FTH_TYPE_ERROR = 0,
    FTH_TYPE_INTEGER,
    FTH_TYPE_FLOAT,
    FTH_TYPE_STRING,
} fth_type_t;

typedef struct {
    fth_type_t type;
    union {
        fth_int_t tinteger;
        fth_float_t tfloat;
        const char *tstring;
    } value;
} fth_value_t;

typedef struct fth_entry_t {
    fth_value_t value;
    struct fth_entry_t *next;
} fth_entry_t;

typedef struct {
    fth_entry_t *head, *tail;
} fth_tree_t;

typedef struct {
    struct {
        fth_value_t *entries;
        int head, length;
    } stack;
    struct {
        const char *str;
        int cursor, eof;
        size_t length;
        fth_tree_t ast;
    } lexer;
} fth_state_t;

EXPORT void fth_init(fth_state_t *fth, int max_stack_size);
EXPORT void fth_destroy(fth_state_t *fth);
EXPORT int fth_parse(fth_state_t *fth, const char *str, size_t str_length);
EXPORT int fth_parse_file(fth_state_t *fth, const char *path);
EXPORT int fth_eval_step(fth_state_t *fth, int *is_stack_empty);
EXPORT int fth_eval(fth_state_t *fth);

#undef EXPORT

#if defined(__cplusplus)
}
#endif
#endif // FTH_HEADER
