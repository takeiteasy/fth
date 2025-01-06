#ifndef FTH_HEADER
#define FTH_HEADER
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <wchar.h>

typedef struct fth_chunk fth_chunk;

typedef enum {
    FTH_VALUE_NIL,
    FTH_VALUE_BOOL,
    FTH_VALUE_INTEGER,
    FTH_VALUE_NUMBER
} fth_value_t;

#define FTH_NIL_VAL ((fth_value){FTH_VALUE_NIL, {0}})
#define FTH_BOOL_VAL(value) ((fth_value){FTH_VALUE_BOOL, {.boolean = value}})
#define FTH_INTEGER_VAL(value) ((fth_value){FTH_VALUE_INTEGER, {.integer = value}})
#define FTH_NUMBER_VAL(value) ((fth_value){FTH_VALUE_NUMBER, {.number = value}})

#define FTH_IS_NIL(value) ((value).type == FTH_VALUE_NIL)
#define FTH_IS_BOOL(value) ((value).type == FTH_VALUE_BOOL)
#define FTH_IS_INTEGER(value) ((value).type == FTH_VALUE_INTEGER)
#define FTH_IS_NUMBER(value) ((value).type == FTH_VALUE_NUMBER)

#define FTH_AS_BOOL(value) ((value).as.boolean)
#define FTH_AS_INTEGER(value) ((value).as.integer)
#define FTH_AS_NUMBER(value) ((value).as.number)

typedef struct {
    fth_value_t type;
    union {
        uint8_t boolean;
        uint64_t integer;
        double number;
    } as;
} fth_value;

typedef struct {
    fth_chunk *chunk;
    uint8_t *pc;
    fth_value *stack;
    fth_value *return_stack;
    fth_value current;
    fth_value previous;
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
