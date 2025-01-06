#include "fth.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define __garry_raw(a)           ((int*)(void*)(a)-2)
#define __garry_m(a)             __garry_raw(a)[0]
#define __garry_n(a)             __garry_raw(a)[1]
#define __garry_needgrow(a,n)    ((a)==0 || __garry_n(a)+(n) >= __garry_m(a))
#define __garry_maybegrow(a,n)   (__garry_needgrow(a,(n)) ? __garry_grow(a,n) : 0)
#define __garry_grow(a,n)        (*((void **)&(a)) = __garry_growf((a), (n), sizeof(*(a))))
#define __garry_needshrink(a)    (__garry_m(a) > 4 && __garry_n(a) <= __garry_m(a) / 4)
#define __garry_maybeshrink(a)   (__garry_needshrink(a) ? __garry_shrink(a) : 0)
#define __garry_shrink(a)        (*((void **)&(a)) = __garry_shrinkf((a), sizeof(*(a))))

#define garry_free(a)           ((a) ? free(__garry_raw(a)),((a)=NULL) : 0)
#define garry_append(a,v)       (__garry_maybegrow(a,1), (a)[__garry_n(a)++] = (v))
#define garry_insert(a, idx, v) (__garry_maybegrow(a,1), memmove(&a[idx+1], &a[idx], (__garry_n(a)++ - idx) * sizeof(*(a))), a[idx] = (v))
#define garry_push(a,v)         (garry_insert(a,0,v))
#define garry_count(a)          ((a) ? __garry_n(a) : 0)
#define garry_reserve(a,n)      (__garry_maybegrow(a,n), __garry_n(a)+=(n), &(a)[__garry_n(a)-(n)])
#define garry_cdr(a)            (void*)(garry_count(a) > 1 ? &(a+1) : NULL)
#define garry_car(a)            (void*)((a) ? &(a)[0] : NULL)
#define garry_last(a)           (void*)((a) ? &(a)[__garry_n(a)-1] : NULL)
#define garry_pop(a)            (--__garry_n(a), __garry_maybeshrink(a))
#define garry_remove_at(a, idx) (idx == __garry_n(a)-1 ? memmove(&a[idx], &arr[idx+1], (--__garry_n(a) - idx) * sizeof(*(a))) : garry_pop(a), __garry_maybeshrink(a))
#define garry_shift(a)          (garry_remove_at(a, 0))
#define garry_clear(a)          ((a) ? (__garry_n(a) = 0) : 0, __garry_shrink(a))

static void *__garry_growf(void *arr, int increment, int itemsize) {
    int dbl_cur = arr ? 2 * __garry_m(arr) : 0;
    int min_needed = garry_count(arr) + increment;
    int m = dbl_cur > min_needed ? dbl_cur : min_needed;
    int *p = realloc(arr ? __garry_raw(arr) : 0, itemsize * m + sizeof(int) * 2);
    if (p) {
        if (!arr)
            p[1] = 0;
        p[0] = m;
        return p + 2;
    }
    return NULL;
}

static void *__garry_shrinkf(void *arr, int itemsize) {
    int new_capacity = __garry_m(arr) / 2;
    int *p = realloc(arr ? __garry_raw(arr) : 0, itemsize * new_capacity + sizeof(int) * 2);
    if (p) {
        p[0] = new_capacity;
        return p + 2;
    }
    return NULL;
}

static char* __format(const char *fmt, size_t *size, va_list args) {
    va_list _copy;
    va_copy(_copy, args);
    size_t _size = vsnprintf(NULL, 0, fmt, args);
    va_end(_copy);
    char *result = malloc(_size + 1);
    if (!result)
        return NULL;
    vsnprintf(result, _size, fmt, args);
    result[_size] = '\0';
    if (size)
        *size = _size;
    return result;
}

static char* format(const char *fmt, size_t *size, ...) {
    va_list args;
    va_start(args, size);
    size_t length;
    char* result = __format(fmt, &length, args);
    va_end(args);
    if (size)
        *size = length;
    return result;
}

fth_value fth_nil(void) {
    return (fth_value) {
        .type = FTH_VALUE_NIL
    };
}

#define X(T, N, TYPE) \
fth_value fth_##N(TYPE v) { \
    return (fth_value) { \
        .type = FTH_VALUE_##T, \
        .as.N = v \
    }; \
} \
bool fth_is_##N(fth_value v) { \
    return v.type == FTH_VALUE_##T; \
} \
TYPE fth_as_##N(fth_value v) { \
    return (TYPE)v.as.N; \
}
TYPES
#undef X

bool fth_object_is(fth_value value, fth_object_t type) {
    return fth_is_obj(value) && ((fth_object*)fth_as_obj(value))->type == type;
}

fth_object* fth_obj_new(fth_object_t type, size_t size) {
    fth_object *result = malloc(size);
    result->type = type;
    return result;
}

void fth_obj_destroy(fth_object *obj) {
    switch (obj->type) {
        case FTH_OBJECT_STRING: {
            fth_string* string = (fth_string*)obj;
            free(string->chars);
            free(string);
            break;
        }
    }
}

fth_string* fth_copy_string(const unsigned char *chars, int length) {
    fth_string *result = malloc(sizeof(fth_string));
    result->obj.type = FTH_OBJECT_STRING;
    if (!chars) {
        result->chars = malloc(1);
        result->chars[0] = '\0';
        result->length = 0;
    } else {
        result->length = length ? length : (int)strlen((const char*)chars);
        size_t size = length * sizeof(unsigned char);
        result->chars = malloc(size + 1);
        memcpy(result->chars, chars, size);
    }
    return result;
}

void fth_print_value(fth_value value) {
    switch (value.type) {
        case FTH_VALUE_NIL:
            printf("NIL");
            break;
        case FTH_VALUE_BOOLEAN:
            printf("%s", value.as.boolean ? "TRUE" : "FALSE");
            break;
        case FTH_VALUE_INTEGER:
            printf("%llu", fth_as_integer(value));
            break;
        case FTH_VALUE_NUMBER:
            printf("%g", fth_as_number(value));
            break;
        case FTH_VALUE_OBJECT: {
            fth_object *obj = fth_as_obj(value);
            switch (obj->type) {
                case FTH_OBJECT_STRING:
                    printf("\"%.*s\"", fth_string_length(value), fth_as_cstring(value));
                    break;
                default:
                    abort();
            }
            break;
        }
        default:
            abort();
    }
}

#include "chunk.inl"
#include "lexer.inl"

static fth_result_t fth_run(fth_vm *vm) {
    for (;;) {
        uint8_t instruction;
        switch (instruction = *vm->pc++) {
            case FTH_OP_RETURN: {
                fth_value value;
                fth_result_t result;
                if ((result = fth_stack_pop(vm, &value)) != FTH_OK) {
                    vm->error = strdup("stack underflow");
                    return result;
                }
                fth_print_value(value);
                printf("\n");
                return FTH_OK;
            }
            case FTH_OP_CONSTANT:
            case FTH_OP_CONSTANT_LONG:
                fth_stack_push(vm, vm->chunk->constants[*vm->pc++]);
                break;
            case FTH_OP_PERIOD:
                if (!garry_count(vm->stack)) {
                    vm->error = strdup("stack underflow");
                    return FTH_RUNTIME_ERROR;
                }
                fth_print_value(*(fth_value*)garry_last(vm->stack));
                printf("\n");
                break;
            case FTH_OP_DUMP:
                for (int i = 0; i < garry_count(vm->stack); i++) {
                    printf("[ ");
                    fth_print_value(vm->stack[i]);
                    printf(" ]");
                }
                printf("\n");
                break;
            default:
                abort();
        }
    }
    return FTH_OK;
}

static void stack_reset(fth_vm *vm) {
    garry_free(vm->stack);
    vm->stack = NULL;
    garry_free(vm->return_stack);
    vm->return_stack = NULL;
}

void fth_init(fth_vm *vm) {
    memset(vm, 0, sizeof(fth_vm));
    stack_reset(vm);
}

void fth_destroy(fth_vm *vm) {
    if (vm->error)
        free(vm->error);
    for (int i = 0; i < garry_count(vm->objects); i++)
        if (vm->objects[i].type == FTH_VALUE_OBJECT)
            fth_obj_destroy(&vm->objects[i]);
    garry_free(vm->objects);
    stack_reset(vm);
}

void fth_stack_push(fth_vm *vm, fth_value value) {
    garry_append(vm->stack, value);
}

fth_result_t fth_stack_pop(fth_vm *vm, fth_value *value) {
    if (!garry_count(vm->stack)) {
        if (value)
            *value = fth_nil();
        return FTH_RUNTIME_ERROR;
    } else {
        if (value)
            *value = *(fth_value*)garry_last(vm->stack);
        garry_pop(vm->stack);
        return FTH_OK;
    }
}

fth_result_t fth_stack_at(fth_vm *vm, int idx, fth_value *value) {
    return FTH_OK;
}

fth_result_t fth_stack_peek(fth_vm *vm, int distance, fth_value *value) {
    return FTH_OK;
}

fth_result_t fth_exec(fth_vm *vm, const unsigned char *source) {
    fth_result_t result = FTH_OK;
    fth_chunk chunk;
    chunk_init(&chunk);
    fth_parser parser;
    parser_init(&parser, source);
    if (fth_compile(&parser, &chunk) != FTH_OK) {
        result = FTH_COMPILE_ERROR;
        goto BAIL;
    }
    vm->chunk = &chunk;
    vm->pc = vm->chunk->data;
    result = fth_run(vm);
BAIL:
    chunk_free(&chunk);
    return result;
}

static unsigned char* readfile(const char *path, size_t *size) {
    unsigned char *result = NULL;
    size_t _size = 0;
    FILE *file = fopen(path, "r");
    if (!file)
        goto BAIL; // _size = 0 failed to open file
    fseek(file, 0, SEEK_END);
    _size = ftell(file);
    rewind(file);
    if (!(result = malloc(sizeof(unsigned char) * _size + 1)))
        goto BAIL; // _size > 0 failed to alloc memory
    if (fread(result, sizeof(unsigned char), _size, file) != _size) {
        free(result);
        _size = -1;
        goto BAIL; // _size = -1 failed to read file
    }
    result[_size] = '\0';
BAIL:
    if (size)
        *size = _size;
    return result;
}

fth_result_t fth_exec_file(fth_vm *vm, const char *path) {
    size_t size;
    unsigned char *source = readfile(path, &size);
    if (!source)
        switch (size) {
            case 0:
                vm->error = format("failed to open '%s'\n", NULL, path);
                return FTH_COMPILE_ERROR;
            case -1:
                vm->error = format("failed to read '%s'\n", NULL, path);
                return FTH_COMPILE_ERROR;
            default:
                vm->error = format("failed to alloc memory '%zub'\n", NULL, size);
                return FTH_COMPILE_ERROR;
        }
    fth_result_t result = fth_exec(vm, source);
    free(source);
    return result;
}
