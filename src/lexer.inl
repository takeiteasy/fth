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
    FTH_TOKEN_POP,  // >$
    FTH_TOKEN_PUSH, //  $>
    FTH_TOKEN_DUMP  // .$
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

static int ctowc(const unsigned char* c, wchar_t* out) {
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
    return peek(parser) == L'\0';
}

static inline void update_start(fth_parser *parser) {
    parser->begin = parser->cursor.ptr;
}

static inline wchar_t advance(fth_parser *parser) {
    int l = ctowc(parser->cursor.ptr, NULL);
    parser->cursor.ptr += l;
    parser->cursor.ch_length = ctowc(parser->cursor.ptr, &parser->cursor.ch);
    if (parser->cursor.ch == L'\n')
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
        return L'\0';
    wchar_t next;
    ctowc(parser->cursor.ptr + parser->cursor.ch_length, &next);
    return next;
}

static void skip_line(fth_parser *parser) {
    for (;;) {
        if (is_eof(parser))
            return;
        switch (peek(parser)) {
            case L'\r':
                switch (next(parser)) {
                    case L'\n':
                        advance(parser);
                    case L'\0':
                        advance(parser);
                        return;
                    default:
                        break;
                }
                break;
            case L'\n':
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
            case L'#':
                skip_line(parser);
                break;
            case L' ':
            case L'\t':
            case L'\v':
            case L'\r':
            case L'\n':
            case L'\f':
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
            case L'#':
            case L' ':
            case L'\t':
            case L'\v':
            case L'\r':
            case L'\n':
            case L'\f':
            case L'(':
            case L')':
            case L'[':
            case L']':
            case L'{':
            case L'}':
            case L';':
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
            case L'0' ... L'9':
                advance(parser);
                break;
            case L'.':
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
    switch (peek(parser)) {
        case L' ':
        case L'\t':
        case L'\v':
        case L'\r':
        case L'\n':
        case L'\f':
            skip_whitespace(parser);
            update_start(parser);
            return next_token(parser);
        case L'#':
            skip_line(parser);
            update_start(parser);
            return next_token(parser);
        case L'0' ... L'9':
            return read_number(parser);
        case L'"':
            return read_string(parser);
        case L'.':;
            if (next(parser) == L'$') {
                advance_n(parser, 2);
                return fth_token_make(parser, FTH_TOKEN_DUMP);
            } else {
                advance(parser);
                return fth_token_make(parser, FTH_TOKEN_PERIOD);
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
        case FTH_TOKEN_POP:  // >$
            return "STACK_POP";
        case FTH_TOKEN_PUSH: //  $>
            return "STACK_PUSH";
        case FTH_TOKEN_DUMP:  // .$
            return "STACK_DUMP";
    }
}

static void fth_print_token(fth_token *token) {
    printf("[%s] %.*s\n", fth_token_str(token), token->length, token->begin);
}

static void parser_init(fth_parser *parser, const unsigned char *source) {
    memset(parser, 0, sizeof(fth_parser));
    parser->begin = source;
    parser->cursor.ptr = source;
    ctowc(source, &parser->cursor.ch);
    parser->line = 1;
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
        switch (parser->current.type) {
            case FTH_TOKEN_EOF:
                emit(parser, chunk, FTH_TOKEN_EOF);
            case FTH_TOKEN_ERROR:
                goto BAIL;
            case FTH_TOKEN_ATOM:
                break;
            case FTH_TOKEN_STRING:
                emit_constant(parser, chunk, fth_obj(fth_copy_string(parser->current.begin, parser->current.length)));
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
                break;
            case FTH_TOKEN_PUSH:
                break;
            case FTH_TOKEN_DUMP:
                emit(parser, chunk, FTH_OP_DUMP);
                break;
        }
        parser->previous = parser->current;
    }
BAIL:
    return parser->error == NULL ? FTH_OK : FTH_COMPILE_ERROR;
}
