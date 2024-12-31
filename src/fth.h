#ifndef FTH_HEADER
#define FTH_HEADER
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

typedef double fth_value;

typedef struct {
    int offset, line;
} fth_chunk_linestart;

typedef struct {
    uint8_t *data;
    fth_value *constants;
    fth_chunk_linestart *lines;
} fth_chunk;

typedef struct {
    fth_chunk *chunk;
    uint8_t *pc;
    fth_value *stack;
} fth_vm;

typedef enum {
    FTH_OK,
    FTH_COMPILE_ERROR,
    FTH_RUNTIME_ERROR
} fth_result_t;

typedef enum {
    FTH_RETURN,
    FTH_CONSTANT,
    FTH_CONSTANT_LONG
} fth_op;

void fth_init(fth_vm *vm);
void fth_deinit(fth_vm *vm);

void fth_push(fth_vm *vm, fth_value value);
fth_value fth_pop(fth_vm *vm);

fth_result_t rth_interpret(fth_vm *vm, fth_chunk *chunk);
void fth_compile(const char *src, fth_chunk *chunk);
fth_result_t fth_run(fth_vm *vm);

#ifdef __cplusplus
}
#endif
#endif // FTH_HEADER

#ifdef FTH_IMPL
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

static void chunk_init(fth_chunk *chunk) {
    chunk->data = NULL;
    chunk->constants = NULL;
    chunk->lines = NULL;
}

static void chunk_free(fth_chunk *chunk) {
    garry_free(chunk->data);
    garry_free(chunk->constants);
    garry_free(chunk->lines);
    memset(chunk, 0, sizeof(fth_chunk));
}

static void chunk_write(fth_chunk *chunk, uint8_t byte, int line) {
    garry_append(chunk->data, byte);
    if (chunk->lines && garry_count(chunk->lines) > 0) {
        fth_chunk_linestart *linestart = garry_last(chunk->lines);
        if (linestart && linestart->line == line)
            return;
    }
    fth_chunk_linestart result = {
        .offset = garry_count(chunk->data) - 1,
        .line = line
    };
    garry_append(chunk->lines, result);
}

static int constant_instruction(const char *name, fth_chunk *chunk, int offset) {
    uint8_t constant = chunk->data[offset + 1];
    printf("%-16s %4d '", name, constant);
    printf("%g", chunk->constants[constant]);
    printf("'\n");
    return offset + 2;
}

static int long_constant_instruction(const char *name, fth_chunk *chunk, int offset) {
    uint32_t c = chunk->data[offset + 1] | (chunk->data[offset + 2] << 8) | (chunk->data[offset + 3] << 16);
    printf("%-16s %4d '%g'\n", name, c, chunk->constants[c]);
    return offset + 4;
}

static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int get_line(fth_chunk *chunk, int instruction) {
    int start = 0;
    int end = garry_count(chunk->lines) - 1;
    for (;;) {
        int mid = (start + end) / 2;
        fth_chunk_linestart *line = &chunk->lines[mid];
        if (instruction < line->offset)
            end = mid - 1;
        else if (mid == garry_count(chunk->lines) - 1 || instruction < chunk->lines[mid + 1].offset)
            return line->line;
        else
            start = mid + 1;
    }
}

static int disassemble_instruction(fth_chunk *chunk, int offset) {
    printf("%04d ", offset);
    int line = get_line(chunk, offset);
    if (offset > 0 && line == get_line(chunk, offset - 1))
        printf("   | ");
    else
        printf("%4d ", line);
    uint8_t instruction = chunk->data[offset];
    switch (instruction) {
        case FTH_RETURN:
            return simple_instruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

static void chunk_disassemble(fth_chunk *chunk, const char *name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < garry_count(chunk->data);)
        offset = disassemble_instruction(chunk, offset);
}

static int fth_constant(fth_chunk *chunk, fth_value value) {
    garry_append(chunk->constants, value);
    return garry_count(chunk->constants) - 1;
}

static void chunk_write_constant(fth_chunk *chunk, fth_value value, int line) {
    int index = fth_constant(chunk, value);
    if (index < 256) {
        chunk_write(chunk, FTH_CONSTANT, line);
        chunk_write(chunk, (uint8_t)index, line);
    } else {
        chunk_write(chunk, FTH_CONSTANT_LONG, line);
        chunk_write(chunk, (uint8_t)(index & 0xff), line);
        chunk_write(chunk, (uint8_t)((index >> 8) & 0xff), line);
        chunk_write(chunk, (uint8_t)((index >> 16) & 0xff), line);
    }
}

static void stack_reset(fth_vm *vm) {
    garry_free(vm->stack);
    vm->stack = NULL;
}

void fth_init(fth_vm *vm) {
    stack_reset(vm);
}

void fth_deinit(fth_vm *vm) {
    stack_reset(vm);
}

void fth_push(fth_vm *vm, fth_value value) {
    garry_append(vm->stack, value);
}

fth_value fth_pop(fth_vm *vm) {
    fth_value result = *(fth_value*)garry_last(vm->stack);
    garry_pop(vm->stack);
    return result;
}

fth_result_t fth_interpret(fth_vm *vm, fth_chunk *chunk) {
    vm->chunk = chunk;
    vm->pc = vm->chunk->data;
    return fth_run(vm);
}

void fth_compile(const char *src, fth_chunk *chunk) {
    
}

fth_result_t fth_run(fth_vm *vm) {
    for (;;) {
        printf("          ");
        for (int i = 0; i < garry_count(vm->stack); i++)
            printf("[ %g ]", vm->stack[i]);
        printf("\n");
        disassemble_instruction(vm->chunk, (int)(vm->pc - vm->chunk->data));
        
        uint8_t instruction;
        fth_value v;
        switch (instruction = *vm->pc++) {
            case FTH_RETURN:
                return FTH_OK;
            case FTH_CONSTANT:
                v = vm->chunk->constants[*vm->pc++];
                printf(" %g\n", v);
                break;
            default:
                abort();
        }
    }
}
#endif //FTH_IMPL
