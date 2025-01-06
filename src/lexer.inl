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
    FTH_TOKEN_COLON, // :
    FTH_TOKEN_SEMICOLON, // ;
    FTH_TOKEN_UNARY_NEGATIVE, // -
    FTH_TOKEN_PERIOD, // .
    FTH_TOKEN_LEFT_PAREN, // (
    FTH_TOKEN_RIGHT_PAREN, // )
    FTH_TOKEN_LEFT_SQR_PAREN, // [
    FTH_TOKEN_RIGHT_SQR_PAREN, // ]
    FTH_TOKEN_LEFT_BRKT_PAREN, // {
    FTH_TOKEN_RIGHT_BRKT_PAREN, // }
    // long ops
    FTH_TOKEN_NTH,
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
    ctowc(parser->cursor.ptr, &parser->cursor.ch);
    if (parser->cursor.ch == L'\n')
        parser->line++;
    return parser->cursor.ch;
}

static inline wchar_t next(fth_parser *parser) {
    if (is_eof(parser))
        return L'\0';
    wchar_t next;
    ctowc(parser->cursor.ptr, &next);
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
        case L':':
            return fth_token_make(parser, FTH_TOKEN_COLON);
        case L';':
            return fth_token_make(parser, FTH_TOKEN_SEMICOLON);
        case L'.':
            switch (next(parser)) {
                case L'$':
                    advance(parser);
                    return fth_token_make(parser, FTH_TOKEN_DUMP);
                default:
                    return fth_token_make(parser, FTH_TOKEN_PERIOD);
            }
            break;
        case L'>':
            switch (next(parser)) {
                case L'$':
                    advance(parser);
                    return fth_token_make(parser, FTH_TOKEN_POP);
                default:
                    return read_atom(parser);
            }
        case L'$':
            switch (next(parser)) {
                case L'>':
                    return fth_token_make(parser, FTH_TOKEN_PUSH);
                default:
                    return read_atom(parser);
            }
            break;
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
        case FTH_TOKEN_COLON: // :
            return "COLON";
        case FTH_TOKEN_SEMICOLON: // ;
            return "SEMICOLON";
        case FTH_TOKEN_UNARY_NEGATIVE: // -
            return "NEGATIVE";
        case FTH_TOKEN_PERIOD: // .
            return "PERIOD";
        case FTH_TOKEN_LEFT_PAREN: // (
            return "LEFT_PAREN";
        case FTH_TOKEN_RIGHT_PAREN: // )
            return "RIGHT_PAREN";
        case FTH_TOKEN_LEFT_SQR_PAREN: // [
            return "LEFT_SQUARE_PAREN";
        case FTH_TOKEN_RIGHT_SQR_PAREN: // ]
            return "RIGHT_SQUARE_PAREN";
        case FTH_TOKEN_LEFT_BRKT_PAREN: // {
            return "LEFT_CURLY_PAREN";
        case FTH_TOKEN_RIGHT_BRKT_PAREN: // }
            return "RIGHT_CURLY_PAREN";
        case FTH_TOKEN_NTH:
            return "STACK_AT";
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
