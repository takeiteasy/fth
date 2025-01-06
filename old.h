#ifndef FTH_HEADER
#define FTH_HEADER
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

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

#define FTH_IS_NIL(value) ((value).as.integer == 0 && (value).type == FTH_VALUE_NIL)
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
} fth_vm;

typedef enum {
    FTH_OK,
    FTH_COMPILE_ERROR,
    FTH_RUNTIME_ERROR
} fth_result_t;

void fth_init(fth_vm *vm);
void fth_deinit(fth_vm *vm);

void fth_push(fth_vm *vm, fth_value value);
fth_result_t fth_pop(fth_vm *vm, fth_value *value);

fth_result_t fth_interpret(fth_vm *vm, const char *source);
fth_result_t fth_interpret_file(fth_vm *vm, const char *path);
fth_result_t fth_stack_at(fth_vm *vm, fth_value *value);

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

typedef struct {
    int offset, line;
} fth_chunk_linestart;

struct fth_chunk {
    uint8_t *data;
    fth_value *constants;
    fth_chunk_linestart *lines;
};

typedef enum {
    FTH_OP_RETURN,
    FTH_OP_CONSTANT,
    FTH_OP_CONSTANT_LONG,
    FTH_OP_NEGATE
} fth_op;

typedef enum {
    FTH_TOKEN_ERROR,
    FTH_TOKEN_EOF,
    // literals
    FTH_TOKEN_ATOM,
    FTH_TOKEN_STRING,
    FTH_TOKEN_NUMBER,
    FTH_TOKEN_INTEGER,
    // basic ops
    FTH_TOKEN_COLON,
    FTH_TOKEN_SEMICOLON,
    FTH_TOKEN_UNARY_NEGATIVE,
    FTH_TOKEN_PERIOD,
    FTH_TOKEN_LEFT_PAREN,
    FTH_TOKEN_RIGHT_PAREN,
    FTH_TOKEN_LEFT_SQR_PAREN,
    FTH_TOKEN_RIGHT_SQR_PAREN,
    FTH_TOKEN_LEFT_BRKT_PAREN,
    FTH_TOKEN_RIGHT_BRKT_PAREN,
    // long ops
    FTH_TOKEN_NTH,
    FTH_TOKEN_POP,  // >$
    FTH_TOKEN_PUSH, //  $>
    FTH_TOKEN_DUMP  // .$
} fth_token_t;

typedef struct {
    fth_token_t type;
    const char *begin;
    int length;
    int line;
} fth_token;

typedef struct {
    const char *begin;
    const char *cursor;
    int line;
    fth_token current;
    fth_token previous;
    int error;
} fth_parser;

static void fth_print_value(fth_value value) {
    switch (value.type) {
        case FTH_VALUE_NIL:
            printf("NIL");
            break;
        case FTH_VALUE_BOOL:
            printf("%s", value.as.boolean ? "TRUE" : "FALSE");
            break;
        case FTH_VALUE_INTEGER:
            printf("%llu", FTH_AS_INTEGER(value));
            break;
        case FTH_VALUE_NUMBER:
            printf("%g", FTH_AS_NUMBER(value));
            break;
        default:
            break;
    }
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
    uint8_t c = chunk->data[offset + 1];
    printf("%-16s %4d '", name, c);
    fth_print_value(chunk->constants[c]);
    printf("'\n");
    return offset + 2;
}

static int long_constant_instruction(const char *name, fth_chunk *chunk, int offset) {
    uint32_t c = chunk->data[offset + 1] | (chunk->data[offset + 2] << 8) | (chunk->data[offset + 3] << 16);
    printf("%-16s %4d '", name, c);
    fth_print_value(chunk->constants[c]);
    printf("'\n");
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
        case FTH_OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        case FTH_OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case FTH_OP_CONSTANT_LONG:
            return long_constant_instruction("OP_CONSTANT_LONG", chunk, offset);
        case FTH_OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
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

static int chunk_add_constant(fth_chunk *chunk, fth_value value) {
    garry_append(chunk->constants, value);
    return garry_count(chunk->constants) - 1;
}

static void chunk_write_constant(fth_chunk *chunk, fth_value value, int line) {
    int index = chunk_add_constant(chunk, value);
    if (index < 256) {
        chunk_write(chunk, FTH_OP_CONSTANT, line);
        chunk_write(chunk, (uint8_t)index, line);
    } else {
        chunk_write(chunk, FTH_OP_CONSTANT_LONG, line);
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
    memset(vm, 0, sizeof(fth_vm));
    stack_reset(vm);
}

void fth_deinit(fth_vm *vm) {
    stack_reset(vm);
}

void fth_push(fth_vm *vm, fth_value value) {
    garry_append(vm->stack, value);
}

fth_result_t fth_pop(fth_vm *vm, fth_value *value) {
    if (!garry_count(vm->stack)) {
        if (value)
            *value = FTH_NIL_VAL;
        return FTH_RUNTIME_ERROR;
    } else {
        if (value)
            *value = *(fth_value*)garry_last(vm->stack);
        garry_pop(vm->stack);
        return FTH_OK;
    }
}

fth_result_t fth_stack_at(fth_vm *vm, fth_value *value) {
    return FTH_OK;
}

static fth_token fth_token_make(fth_parser *parser, fth_token_t type) {
    return (fth_token) {
        .type = type,
        .begin = parser->begin,
        .length = (int)(parser->cursor - parser->begin),
        .line = parser->line
    };
}

static fth_token fth_token_error(fth_parser *parser, const char *message) {
    return (fth_token) {
        .type = FTH_TOKEN_ERROR,
        .begin = strdup(message),
        .length = (int)strlen(message),
        .line = parser->line
    };
}

static char* _vsprintf(const char *format, size_t *size, va_list args) {
    va_list _copy;
    va_copy(_copy, args);
    size_t _size = vsnprintf(NULL, 0, format, args);
    va_end(_copy);
    char *result = malloc(_size + 1);
    if (!result)
        return NULL;
    vsnprintf(result, _size, format, args);
    result[_size] = '\0';
    if (size)
        *size = _size;
    return result;
}

static inline fth_token fth_token_error_va(fth_parser *parser, const char *message, ...) {
    va_list args;
    va_start(args, message);
    size_t length;
    char* result = _vsprintf(message, &length, args);
    va_end(args);
    return (fth_token) {
        .type = FTH_TOKEN_ERROR,
        .begin = result,
        .length = (int)length,
        .line = parser->line
    };
}

static inline void update_start(fth_parser *parser) {
    parser->begin = parser->cursor;
}

static inline char peek(fth_parser *parser) {
    return *parser->cursor;
}

static inline int is_eof(fth_parser *parser) {
    return peek(parser) == '\0';
}

static inline char advance(fth_parser *parser) {
    parser->cursor++;
    char c = parser->cursor[-1];
    if (c == '\n')
        parser->line++;
    return c;
}

#define ADVANCE(N) \
    do { \
        for (int i = 0; i < (N); i++) \
            advance(parser); \
    } while(0)

static inline char next(fth_parser *parser) {
    return is_eof(parser) ? '\0' : parser->cursor[1];
}

static inline int match(fth_parser *parser, const char *str) {
    return strncmp(parser->cursor, str, strlen(str));
}

static void skip_line(fth_parser *parser) {
    for (;;) {
        if (is_eof(parser))
            return;
        switch (peek(parser)) {
            case '\r':
                switch (next(parser)) {
                    case '\n':
                        advance(parser);
                    case '\0':
                        advance(parser);
                        return;
                    default:
                        break;
                }
                break;
            case '\n':
                advance(parser);
                return;
        }
        advance(parser);
    }
}

static void skip_whitespace(fth_parser *parser) {
    for (;;) {
        if (is_eof(parser))
            return;
        switch (peek(parser)) {
            case '#':
                skip_line(parser);
                break;
            case ' ':
            case '\t':
            case '\v':
            case '\r':
            case '\n':
            case '\f':
                advance(parser);
                break;
            default:
                return;
        }
    }
}

static inline int upcase(char c) {
    return c >= 'a' && c <= 'z' ? c - 32 : c;
}

static int strncmp_upcase(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2 || !n)
        return -1;
    register unsigned char u1, u2;
    while (n-- > 0) {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (upcase(u1) != upcase(u2))
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }
    return 0;
}

static fth_token symbol_token(fth_parser *parser) {
    for (;;) {
        if (is_eof(parser))
            break;
        switch (peek(parser)) {
            case 'a' ... 'z':
            case '?' ... 'Z':
            case '_':
            case '0' ... '9':
                advance(parser);
                break;
            default:
                goto BREAK;
        }
    }
BREAK:
    return fth_token_make(parser, FTH_TOKEN_ATOM);
}

static fth_token number_token(fth_parser *parser) {
    int is_float = 0;
    for (;;) {
        if (is_eof(parser))
            break;
        switch (peek(parser)) {
            case '0' ... '9':
                advance(parser);
                break;
            case '.':
                if (is_float)
                    return fth_token_error(parser, "multiple decimal place marking");
                is_float = 1;
                advance(parser);
                break;
            default:
                goto DONE;
        }
        
    }
DONE:
    return fth_token_make(parser, is_float ? FTH_TOKEN_NUMBER : FTH_TOKEN_INTEGER);
}

static fth_token string_token(fth_parser *parser) {
    advance(parser); // skip first "
    update_start(parser);
    for (;;) {
        if (is_eof(parser))
            return fth_token_error(parser, "unterminated string"); // unterminated string
        switch (peek(parser)) {
            case '"':;
                fth_token token = fth_token_make(parser, FTH_TOKEN_STRING);
                advance(parser);
                return token;
            default:
                advance(parser);
        }
    }
}

static fth_token next_token(fth_parser *parser) {
    skip_whitespace(parser);
    update_start(parser);
    if (is_eof(parser))
        return fth_token_make(parser, FTH_TOKEN_EOF);
    switch (peek(parser)) {
        case '#':
            skip_line(parser);
            break;
        case 'a' ... 'z':
        case '?' ... 'Z':
        case '_':
            return symbol_token(parser);
        case '0' ... '9':
            return number_token(parser);
        case '"':
            return string_token(parser);
#define PARENS \
    X(PAREN, '(', ')') \
    X(SQR_PAREN, '[', ']') \
    X(BRKT_PAREN, '{', '}')
#define X(T, O, C) \
        case O: \
            return fth_token_make(parser, FTH_TOKEN_LEFT_##T); \
        case C: \
            return fth_token_make(parser, FTH_TOKEN_RIGHT_##T);
        PARENS
#undef X
        case ':':
            return fth_token_make(parser, FTH_TOKEN_COLON);
        case ';':
            return fth_token_make(parser, FTH_TOKEN_SEMICOLON);
        case '-':
            return fth_token_make(parser, FTH_TOKEN_UNARY_NEGATIVE);
        case '.':
            switch (next(parser)) {
                case '$':
                    advance(parser);
                    return fth_token_make(parser, FTH_TOKEN_DUMP);
                default:
                    return fth_token_make(parser, FTH_TOKEN_PERIOD);
            }
            break;
        case '>':
            switch (next(parser)) {
                case '$':
                    advance(parser);
                    return fth_token_make(parser, FTH_TOKEN_POP);
                default:
                    return symbol_token(parser);
            }
        case '$':
            switch (next(parser)) {
                case '>':
                    return fth_token_make(parser, FTH_TOKEN_PUSH);
                    break;
                    
                default:
                    break;
            }
            break;
        default:
            return fth_token_error(parser, "unknown token");
    }
    return fth_token_error(parser, "unknown token");
}

static const char* token_string(fth_token_t type) {
    switch (type) {
        case FTH_TOKEN_ATOM:
            return "ATOM";
        case FTH_TOKEN_STRING:
            return "STRING";
        case FTH_TOKEN_NUMBER:
            return "NUMBER";
        case FTH_TOKEN_INTEGER:
            return "INTEGER";
        default:
            return "UNKNOWN";
    }
}

static inline void print_token(fth_token *token) {
    printf("%s: %.*s\n", token_string(token->type), token->length, token->begin);
}

static void consume(fth_parser *parser, fth_token_t type, const char *message) {
    if (parser->current.type == type)
        advance(parser);
    else
        parser->current = fth_token_error(parser, message);
}

static void emit(fth_parser *parser, fth_chunk *chunk, uint8_t byte) {
    chunk_write(chunk, byte, parser->previous.line);
}

static void emit_op(fth_parser *parser, fth_chunk *chunk, uint8_t byte1, uint8_t byte2) {
    emit(parser, chunk, byte1);
    emit(parser, chunk, byte2);
}

static void emit_return(fth_parser *parser, fth_chunk *chunk) {
    emit(parser, chunk, FTH_OP_RETURN);
}

static void emit_constant(fth_parser *parser, fth_chunk *chunk, fth_value value) {
    chunk_write_constant(chunk, value, parser->previous.line);
}

static void emit_number(fth_parser *parser, fth_chunk *chunk) {
    static char buf[513];
    memset(buf, 0, sizeof(char) * 513);
    if (parser->current.length >= 512)
        abort();
    memcpy(buf, parser->begin, parser->current.length);
    buf[parser->current.length+1] = '\0';
    double value = strtod(buf, NULL);
    emit_constant(parser, chunk, FTH_NUMBER_VAL(value));
}

static void emit_integer(fth_parser *parser, fth_chunk *chunk) {
    static char buf[21];
    memset(buf, 0, sizeof(char) * 20);
    if (parser->current.length >= 20)
        abort();
    memcpy(buf, parser->begin, parser->current.length);
    buf[parser->current.length+1] = '\0';
    char *end;
    uint64_t value = strtoull(buf, &end, 10);
    emit_constant(parser, chunk, FTH_INTEGER_VAL(value));
}

static int fth_compile(const char *src, fth_chunk *chunk) {
    fth_parser parser = {
        .begin = src,
        .cursor = src,
        .line = 1
    };
    
    for (;;) {
        parser.current = next_token(&parser);
        switch (parser.current.type) {
            default:
                parser.error = 1;
                printf("unexpected token: %.*s\n", parser.current.length, parser.current.begin);
                goto BAIL;
            case FTH_TOKEN_ERROR:
                parser.error = 1;
                printf("%s\n", parser.current.begin);
                free((void*)parser.current.begin);
                goto BAIL;
            case FTH_TOKEN_EOF:
                emit(&parser, chunk, FTH_TOKEN_EOF);
                goto BAIL;
            case FTH_TOKEN_ATOM:
                break;
            case FTH_TOKEN_STRING:
                break;
            case FTH_TOKEN_NUMBER:
                emit_number(&parser, chunk);
                break;
            case FTH_TOKEN_INTEGER:
                emit_integer(&parser, chunk);
                break;
            case FTH_TOKEN_COLON:
                break;
            case FTH_TOKEN_SEMICOLON:
                break;
            case FTH_TOKEN_UNARY_NEGATIVE:
                break;
            case FTH_TOKEN_PERIOD:
                break;
            case FTH_TOKEN_LEFT_PAREN:
                break;
            case FTH_TOKEN_LEFT_SQR_PAREN:
                break;
            case FTH_TOKEN_LEFT_BRKT_PAREN:
                break;
            case FTH_TOKEN_NTH:
                break;
            case FTH_TOKEN_POP:
                break;
            case FTH_TOKEN_PUSH:
                break;
            case FTH_TOKEN_DUMP:
                break;
        }
        parser.previous = parser.current;
    }
BAIL:
    return parser.error != 1;
}

#define BINARY_OP(vm, op) \
    do { \
      double b = fth_pop(vm); \
      double a = fth_pop(vm); \
      fth_push(vm, a op b); \
    } while(0)

static void runtimeError(fth_vm *vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->pc - vm->chunk->data - 1;
    int line = vm->chunk->lines[instruction].line;
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

static fth_result_t fth_run(fth_vm *vm) {
    for (;;) {
        printf("          ");
        for (int i = 0; i < garry_count(vm->stack); i++) {
            printf("[ ");
            fth_print_value(vm->stack[i]);
            printf("\n");
        }
        printf("\n");
        disassemble_instruction(vm->chunk, (int)(vm->pc - vm->chunk->data));
        
        uint8_t instruction;
        switch (instruction = *vm->pc++) {
            case FTH_OP_RETURN: {
                fth_value value;
                fth_result_t result;
                if ((result = fth_pop(vm, &value)) != FTH_OK)
                    return result;
                printf(" ");
                fth_print_value(value);
                printf("\n");
                return FTH_OK;
            }
            case FTH_OP_CONSTANT:
                fth_push(vm, vm->chunk->constants[*vm->pc++]);
                break;
            case FTH_OP_NEGATE: {
                fth_value* value = garry_last(vm->stack);
                switch (value->type) {
                    case FTH_VALUE_BOOL:
                        value->as.boolean = !value->as.boolean;
                        break;
                    case FTH_VALUE_INTEGER:
                        value->as.integer = -value->as.integer;
                        break;
                    case FTH_VALUE_NUMBER:
                        value->as.number = -value->as.integer;
                        break;
                    default:
                        return FTH_RUNTIME_ERROR;
                }
                break;
            }
            default:
                abort();
        }
    }
}

fth_result_t fth_interpret(fth_vm *vm, const char *source) {
    fth_result_t result = FTH_OK;
    fth_chunk chunk;
    chunk_init(&chunk);
    if (!fth_compile(source, &chunk)) {
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

static char* readfile(const char *path, size_t *size) {
    char *result = NULL;
    size_t _size = 0;
    FILE *file = fopen(path, "r");
    if (!file)
        goto BAIL; // _size = 0 failed to open file
    fseek(file, 0, SEEK_END);
    _size = ftell(file);
    rewind(file);
    if (!(result = malloc(sizeof(char) * _size + 1)))
        goto BAIL; // _size > 0 failed to alloc memory
    if (fread(result, sizeof(char), _size, file) != _size) {
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

fth_result_t fth_interpret_file(fth_vm *vm, const char *path) {
    size_t size;
    char *source = readfile(path, &size);
    if (!source)
        switch (size) {
            case 0:
                fprintf(stderr, "failed to open '%s'\n", path);
                return FTH_COMPILE_ERROR;
            case -1:
                fprintf(stderr, "failed to read '%s'\n", path);
                return FTH_COMPILE_ERROR;
            default:
                fprintf(stderr, "failed to alloc memory '%zub'\n", size);
                return FTH_COMPILE_ERROR;
        }
    fth_result_t result = fth_interpret(vm, source);
    free(source);
    return result;
}
#endif //FTH_IMPL
