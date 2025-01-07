#include "fth.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "utils.inl"

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
            if (string->owns_chars)
                free((char*)string->chars);
            free(string);
            break;
        }
    }
}

fth_string* fth_string_new(const unsigned char *chars, int length, bool owns_chars) {
    fth_string *result = malloc(sizeof(fth_string) + length + 1);
    result->obj.type = FTH_OBJECT_STRING;
    result->length = length;
    if (chars && length) {
        memcpy(result->chars, chars, length * sizeof(unsigned char));
        result->chars[length] = '\0';
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

static void __stack_push(fth_value **stack, fth_value value) {
    garry_append(*stack, value);
}

static fth_result_t __stack_pop(fth_value **stack, fth_value *value) {
    if (!garry_count(*stack)) {
        if (value)
            *value = fth_nil();
        return FTH_RUNTIME_ERROR;
    } else {
        if (value)
            *value = *(fth_value*)garry_last(*stack);
        garry_pop(*stack);
        return FTH_OK;
    }
}

static fth_result_t __stack_at(fth_value **stack, int idx, fth_value *value) {
    return FTH_OK;
}

static fth_result_t __stack_peek(fth_value **stack, int distance, fth_value *value) {
    return FTH_OK;
}

#include "chunk.inl"
#include "lexer.inl"

static void dump_stack(fth_value *stack) {
    for (int i = 0; i < garry_count(stack); i++) {
        fth_print_value(stack[i]);
        printf(" ");
    }
    printf("\n");
}

static fth_result_t fth_run(fth_vm *vm) {
    for (;;) {
        uint8_t instruction;
        fth_value value;
        fth_result_t result;
        switch (instruction = *vm->sp++) {
            case FTH_OP_RETURN:
                if ((result = fth_stack_pop(vm, &value)) != FTH_OK) {
                    vm->error = strdup("stack underflow");
                    return result;
                }
                fth_print_value(value);
                printf("\n");
                return FTH_OK;
            case FTH_OP_CONSTANT:
            case FTH_OP_CONSTANT_LONG:
                __stack_push(&vm->stack, vm->chunk->constants[*vm->sp++]);
                break;
            case FTH_OP_PERIOD:
                if (!garry_count(vm->stack)) {
                    vm->error = strdup("data stack underflow");
                    return FTH_RUNTIME_ERROR;
                }
                fth_print_value(*(fth_value*)garry_last(vm->stack));
                printf("\n");
                break;
            case FTH_OP_PUSH:
                if ((result = fth_stack_pop(vm, &value)) != FTH_OK) {
                    vm->error = strdup("return stack underflow");
                    return result;
                }
                __stack_push(&vm->return_stack, value);
                break;
            case FTH_OP_POP:
                if ((result = __stack_pop(&vm->return_stack, &value)) != FTH_OK) {
                    vm->error = strdup("data stack underflow");
                    return result;
                }
                fth_stack_push(vm, value);
                break;
            case FTH_OP_DUMP:
                dump_stack(vm->stack);
                break;
            case FTH_OP_DUMP_RSTACK:
                dump_stack(vm->return_stack);
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
    __stack_push(&vm->stack, value);
}

fth_result_t fth_stack_pop(fth_vm *vm, fth_value *value) {
    return __stack_pop(&vm->stack, value);
}

fth_result_t fth_stack_at(fth_vm *vm, int idx, fth_value *value) {
    return __stack_at(&vm->stack, idx, value);
}

fth_result_t fth_stack_peek(fth_vm *vm, int distance, fth_value *value) {
    return __stack_peek(&vm->stack, distance, value);
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
    chunk_disassemble(&chunk, "test");
    vm->chunk = &chunk;
    vm->sp = vm->chunk->data;
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
