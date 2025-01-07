//
//  lexer.inl
//  fth
//
//  Created by George Watson on 06/01/2025.
//

typedef enum {
    FTH_TOKEN_ERROR,
    FTH_TOKEN_EOF,
    // literals
    FTH_TOKEN_ATOM,
    FTH_TOKEN_STRING,
    FTH_TOKEN_NUMBER,
    FTH_TOKEN_INTEGER,
    // basic ops
    FTH_TOKEN_PERIOD, // .
    // long ops
    FTH_TOKEN_POP,  //  R>
    FTH_TOKEN_PUSH, // >R
    FTH_TOKEN_DUMP, // .S
    FTH_TOKEN_DUMP_RSTACK // .R
} fth_token_t;

typedef struct {
    fth_token_t type;
    const unsigned char *begin;
    int length;
    int line;
} fth_token;

typedef struct {
    const unsigned char *begin;
    struct {
        const unsigned char *ptr;
        wchar_t ch;
        int ch_length;
    } cursor;
    int line;
    fth_token current;
    fth_token previous;
    const char *error;
} fth_parser;

static int utf8read(const unsigned char* c, wchar_t* out) {
    wchar_t u = *c, l = 1;
    if ((u & 0xC0) == 0xC0) {
        int a = (u & 0x20) ? ((u & 0x10) ? ((u & 0x08) ? ((u & 0x04) ? 6 : 5) : 4) : 3) : 2;
        if (a < 6 || !(u & 0x02)) {
            u = ((u << (a + 1)) & 0xFF) >> (a + 1);
            for (int b = 1; b < a; ++b)
                u = (u << 6) | (c[l++] & 0x3F);
        }
    }
    if (out)
        *out = u;
    return l;
}

static inline wchar_t peek(fth_parser *parser) {
    return parser->cursor.ch;
}

static inline int is_eof(fth_parser *parser) {
    return peek(parser) == '\0';
}

static inline void update_start(fth_parser *parser) {
    parser->begin = parser->cursor.ptr;
}

static inline wchar_t advance(fth_parser *parser) {
    int l = utf8read(parser->cursor.ptr, NULL);
    parser->cursor.ptr += l;
    parser->cursor.ch_length = utf8read(parser->cursor.ptr, &parser->cursor.ch);
    if (parser->cursor.ch == '\n')
        parser->line++;
    return parser->cursor.ch;
}

static inline void advance_n(fth_parser *parser, int _n) {
    int n = _n;
    while (!is_eof(parser) && n-- > 0)
        advance(parser);
}

static inline wchar_t next(fth_parser *parser) {
    if (is_eof(parser))
        return '\0';
    wchar_t next;
    utf8read(parser->cursor.ptr + parser->cursor.ch_length, &next);
    return next;
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

static fth_token fth_token_make(fth_parser *parser, fth_token_t type) {
    return (fth_token) {
        .type = type,
        .begin = parser->begin,
        .length = (int)(parser->cursor.ptr - parser->begin),
        .line = parser->line
    };
}

static fth_token read_atom(fth_parser *parser) {
    for (;;) {
        if (is_eof(parser))
            goto BREAK;
        switch (peek(parser)) {
            case '#':
            case ' ':
            case '\t':
            case '\v':
            case '\r':
            case '\n':
            case '\f':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case ';':
                goto BREAK;
            default:
                advance(parser);
                break;
        }
    }
BREAK:
    return fth_token_make(parser, FTH_TOKEN_ATOM);
}

static fth_token read_number(fth_parser *parser) {
    int is_float = 0;
    for (;;) {
        if (is_eof(parser))
            break;
        switch (peek(parser)) {
            case '0' ... '9':
                advance(parser);
                break;
            case '.':
                if (is_float) {
                    parser->error = "unexpected second '.' in number literal";
                    return fth_token_make(parser, FTH_TOKEN_ERROR);
                }
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

static fth_token read_string(fth_parser *parser) {
    advance(parser); // skip first "
    update_start(parser);
    for (;;) {
        if (is_eof(parser)) {
            parser->error = "unterminated string";
            return fth_token_make(parser, FTH_TOKEN_ERROR); // unterminated string
        }
        switch (peek(parser)) {
            case '"':;
                fth_token token = fth_token_make(parser, FTH_TOKEN_STRING);
                advance(parser); // skip second "
                return token;
            default:
                advance(parser);
        }
    }
}

static fth_token next_token(fth_parser *parser) {
    if (is_eof(parser))
        return fth_token_make(parser, FTH_TOKEN_EOF);
    update_start(parser);
    switch (peek(parser)) {
        case ' ':
        case '\t':
        case '\v':
        case '\r':
        case '\n':
        case '\f':
            skip_whitespace(parser);
            update_start(parser);
            return next_token(parser);
        case '#':
            skip_line(parser);
            update_start(parser);
            return next_token(parser);
        case '0' ... '9':
            return read_number(parser);
        case '"':
            return read_string(parser);
        case '.':
            switch (next(parser)) {
                case 's':
                case 'S':
                    advance_n(parser, 2);
                    return fth_token_make(parser, FTH_TOKEN_DUMP);
                case 'r':
                case 'R':
                    advance_n(parser, 2);
                    return fth_token_make(parser, FTH_TOKEN_DUMP_RSTACK);
                    
                default:
                    advance(parser);
                    return fth_token_make(parser, FTH_TOKEN_PERIOD);
            }
        case '>':
            switch (next(parser)) {
                case 'r':
                case 'R':
                    advance_n(parser, 2);
                    return fth_token_make(parser, FTH_TOKEN_PUSH);
                default:
                    return read_atom(parser);
            }
        case 'r':
        case 'R':
            switch (next(parser)) {
                case '>':
                    advance_n(parser, 2);
                    return fth_token_make(parser, FTH_TOKEN_POP);
                default:
                    return read_atom(parser);
            }
        default:
            return read_atom(parser);
    }
}

static const char* fth_token_str(fth_token *token) {
    switch (token->type) {
        case FTH_TOKEN_ERROR:
            return "ERROR";
        case FTH_TOKEN_EOF:
            return "EOF";
        case FTH_TOKEN_ATOM:
            return "ATOM";
        case FTH_TOKEN_STRING:
            return "STRING";
        case FTH_TOKEN_NUMBER:
            return "NUMBER";
        case FTH_TOKEN_INTEGER:
            return "INTEGER";
        case FTH_TOKEN_PERIOD:
            return "PERIOD";
        case FTH_TOKEN_POP:  // >R
            return "STACK_POP";
        case FTH_TOKEN_PUSH: //  R>
            return "STACK_PUSH";
        case FTH_TOKEN_DUMP:  // .S
            return "STACK_DUMP";
        case FTH_TOKEN_DUMP_RSTACK: // .R
            return "RSTACK_DUMP";
    }
}

static void fth_print_token(fth_token *token) {
    printf("[%s] %.*s\n", fth_token_str(token), token->length, token->begin);
}

static void parser_init(fth_parser *parser, const unsigned char *source) {
    memset(parser, 0, sizeof(fth_parser));
    parser->begin = source;
    parser->cursor.ptr = source;
    utf8read(source, &parser->cursor.ch);
    parser->line = 1;
}

static void emit(fth_parser *parser, fth_chunk *chunk, uint8_t byte) {
    chunk_write(chunk, byte, parser->previous.line);
}

static void emit_op(fth_parser *parser, fth_chunk *chunk, uint8_t byte1, uint8_t byte2) {
    emit(parser, chunk, byte1);
    emit(parser, chunk, byte2);
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
    emit_constant(parser, chunk, fth_number(value));
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
    emit_constant(parser, chunk, fth_integer(value));
}

static fth_result_t fth_compile(fth_parser *parser, fth_chunk *chunk) {
    for (;;) {
        parser->current = next_token(parser);
        fth_print_token(&parser->current);
        switch (parser->current.type) {
            case FTH_TOKEN_EOF:
                emit(parser, chunk, FTH_OP_RETURN);
            case FTH_TOKEN_ERROR:
                goto BAIL;
            case FTH_TOKEN_ATOM:
                break;
            case FTH_TOKEN_STRING:
                emit_constant(parser, chunk, fth_obj(fth_string_new(parser->current.begin, parser->current.length, false)));
                break;
            case FTH_TOKEN_NUMBER:
                emit_number(parser, chunk);
                break;
            case FTH_TOKEN_INTEGER:
                emit_integer(parser, chunk);
                break;
            case FTH_TOKEN_PERIOD:
                emit(parser, chunk, FTH_OP_PERIOD);
                break;
            case FTH_TOKEN_POP:
                emit(parser, chunk, FTH_OP_POP);
                break;
            case FTH_TOKEN_PUSH:
                emit(parser, chunk, FTH_OP_PUSH);
                break;
            case FTH_TOKEN_DUMP:
                emit(parser, chunk, FTH_OP_DUMP);
                break;
            case FTH_TOKEN_DUMP_RSTACK:
                emit(parser, chunk, FTH_OP_DUMP_RSTACK);
                break;
        }
        parser->previous = parser->current;
    }
BAIL:
    return parser->error == NULL ? FTH_OK : FTH_COMPILE_ERROR;
}
