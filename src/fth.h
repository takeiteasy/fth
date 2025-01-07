#ifndef FTH_HEADER
#define FTH_HEADER
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <wchar.h>
#if defined(_MSC_VER) && _MSC_VER < 1800
#include <windef.h>
#define bool BOOL
#define true 1
#define false 0
#else
#if defined(__STDC__) && __STDC_VERSION__ < 199901L
typedef enum bool { false = 0, true = !false } bool;
#else
#include <stdbool.h>
#endif
#endif

typedef uint64_t fth_int;
typedef double fth_float;
typedef struct fth_chunk fth_chunk;

#define TYPES \
    X(BOOLEAN, boolean, bool) \
    X(INTEGER, integer, fth_int) \
    X(NUMBER, number, fth_float) \
    X(OBJECT, obj, void*)

typedef enum {
    FTH_VALUE_NIL,
#define X(T, _, __) FTH_VALUE_##T,
    TYPES
#undef X
} fth_value_t;

typedef struct {
    fth_value_t type;
    union {
#define X(_, N, TYPE) TYPE N;
        TYPES
#undef X
    } as;
} fth_value;

typedef enum {
    FTH_OBJECT_STRING
} fth_object_t;

typedef struct fth_object {
    fth_object_t type;
} fth_object;

typedef struct {
    fth_object obj;
    int length;
    bool owns_chars;
    unsigned char chars[];
} fth_string;

bool fth_object_is(fth_value value, fth_object_t type);
fth_object* fth_obj_new(fth_object_t type, size_t size);
void fth_obj_destroy(fth_object *obj);
fth_string* fth_string_new(const unsigned char *str, int length, bool owns_chars);
#define fth_is_string(VAL) (fth_object_is((VAL), FTH_OBJECT_STRING))
#define fth_as_string(VAL) ((fth_string*)fth_as_obj((VAL)))
#define fth_as_cstring(VAL) ((fth_as_string((VAL)))->chars)
#define fth_string_length(VAL) ((fth_as_string((VAL)))->length)
void fth_print_value(fth_value value);

fth_value fth_nil(void);
#define X(T, N, TYPE) \
fth_value fth_##N(TYPE v);
TYPES
#undef X

typedef struct {
    fth_chunk *chunk;
    uint8_t *sp;
    fth_value *stack;
    fth_value *return_stack;
    fth_value current;
    fth_value previous;
    fth_object *objects;
    char *error;
} fth_vm;

typedef enum {
    FTH_OK,
    FTH_COMPILE_ERROR,
    FTH_RUNTIME_ERROR
} fth_result_t;

void fth_init(fth_vm *vm);
void fth_destroy(fth_vm *vm);

void fth_stack_push(fth_vm *vm, fth_value value);
fth_result_t fth_stack_pop(fth_vm *vm, fth_value *value);
fth_result_t fth_stack_at(fth_vm *vm, int idx, fth_value *value);
fth_result_t fth_stack_peek(fth_vm *vm, int distance, fth_value *value);

fth_result_t fth_exec(fth_vm *vm, const unsigned char *source);
fth_result_t fth_exec_file(fth_vm *vm, const char *path);

#ifdef __cplusplus
}
#endif
#endif // FTH_HEADER
