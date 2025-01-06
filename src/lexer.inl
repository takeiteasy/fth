//
//  lexer.inl
//  fth
//
//  Created by George Watson on 06/01/2025.
//

// is_utf8  Copyright (c) 2013 Palard Julien. All rights reserved. (MIT)
// https://raw.githubusercontent.com/JulienPalard/is_utf8/refs/heads/master/COPYRIGHT

/*
  Check if the given unsigned char * is a valid utf-8 sequence.

  Return value :
  If the string is valid utf-8, 0 is returned.
  Else the position, starting from 1, is returned.

  Source:
   http://www.unicode.org/versions/Unicode7.0.0/UnicodeStandard-7.0.pdf
   page 124, 3.9 "Unicode Encoding Forms", "UTF-8"


  Table 3-7. Well-Formed UTF-8 Byte Sequences
  -----------------------------------------------------------------------------
  |  Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
  |  U+0000..U+007F     |     00..7F |             |            |             |
  |  U+0080..U+07FF     |     C2..DF |      80..BF |            |             |
  |  U+0800..U+0FFF     |         E0 |      A0..BF |     80..BF |             |
  |  U+1000..U+CFFF     |     E1..EC |      80..BF |     80..BF |             |
  |  U+D000..U+D7FF     |         ED |      80..9F |     80..BF |             |
  |  U+E000..U+FFFF     |     EE..EF |      80..BF |     80..BF |             |
  |  U+10000..U+3FFFF   |         F0 |      90..BF |     80..BF |      80..BF |
  |  U+40000..U+FFFFF   |     F1..F3 |      80..BF |     80..BF |      80..BF |
  |  U+100000..U+10FFFF |         F4 |      80..8F |     80..BF |      80..BF |
  -----------------------------------------------------------------------------

  Returns the first erroneous byte position, and give in
  `faulty_bytes` the number of actually existing bytes taking part in this error.
*/
static size_t is_utf8(const unsigned char *str, size_t len, char **message, int *faulty_bytes) {
    size_t i = 0;

    *message = NULL;
    *faulty_bytes = 0;
    while (i < len) {
        if (str[i] <= 0x7F) /* 00..7F */
            i += 1;
        else if (str[i] >= 0xC2 && str[i] <= 0xDF) /* C2..DF 80..BF */
        {
            if (i + 1 < len) /* Expect a 2nd byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0xBF) {
                    *message = "After a first byte between C2 and DF, expecting a 2nd byte between 80 and BF";
                    *faulty_bytes = 2;
                    return i;
                }
            } else {
                *message = "After a first byte between C2 and DF, expecting a 2nd byte.";
                *faulty_bytes = 1;
                return i;
            }
            i += 2;
        }
        else if (str[i] == 0xE0) /* E0 A0..BF 80..BF */
        {
            if (i + 2 < len) /* Expect a 2nd and 3rd byte */
            {
                if (str[i + 1] < 0xA0 || str[i + 1] > 0xBF) {
                    *message = "After a first byte of E0, expecting a 2nd byte between A0 and BF.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte of E0, expecting a 3nd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
            } else {
                *message = "After a first byte of E0, expecting two following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 3;
        }
        else if (str[i] >= 0xE1 && str[i] <= 0xEC) /* E1..EC 80..BF 80..BF */
        {
            if (i + 2 < len) /* Expect a 2nd and 3rd byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0xBF) {
                    *message = "After a first byte between E1 and EC, expecting the 2nd byte between 80 and BF.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte between E1 and EC, expecting the 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
            } else {
                *message = "After a first byte between E1 and EC, expecting two following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 3;
        }
        else if (str[i] == 0xED) /* ED 80..9F 80..BF */
        {
            if (i + 2 < len) /* Expect a 2nd and 3rd byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0x9F) {
                    *message = "After a first byte of ED, expecting 2nd byte between 80 and 9F.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte of ED, expecting 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
            } else {
                *message = "After a first byte of ED, expecting two following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 3;
        }
        else if (str[i] >= 0xEE && str[i] <= 0xEF) /* EE..EF 80..BF 80..BF */
        {
            if (i + 2 < len) /* Expect a 2nd and 3rd byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0xBF) {
                    *message = "After a first byte between EE and EF, expecting 2nd byte between 80 and BF.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte between EE and EF, expecting 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
            } else {
                *message = "After a first byte between EE and EF, two following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 3;
        }
        else if (str[i] == 0xF0) /* F0 90..BF 80..BF 80..BF */
        {
            if (i + 3 < len) /* Expect a 2nd, 3rd 3th byte */
            {
                if (str[i + 1] < 0x90 || str[i + 1] > 0xBF) {
                    *message = "After a first byte of F0, expecting 2nd byte between 90 and BF.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte of F0, expecting 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
                if (str[i + 3] < 0x80 || str[i + 3] > 0xBF) {
                    *message = "After a first byte of F0, expecting 4th byte between 80 and BF.";
                    *faulty_bytes = 4;
                    return i;
                }
            } else {
                *message = "After a first byte of F0, expecting three following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 4;
        }
        else if (str[i] >= 0xF1 && str[i] <= 0xF3) /* F1..F3 80..BF 80..BF 80..BF */
        {
            if (i + 3 < len) /* Expect a 2nd, 3rd 3th byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0xBF) {
                    *message = "After a first byte of F1, F2, or F3, expecting a 2nd byte between 80 and BF.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte of F1, F2, or F3, expecting a 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
                if (str[i + 3] < 0x80 || str[i + 3] > 0xBF) {
                    *message = "After a first byte of F1, F2, or F3, expecting a 4th byte between 80 and BF.";
                    *faulty_bytes = 4;
                    return i;
                }
            } else {
                *message = "After a first byte of F1, F2, or F3, expecting three following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 4;
        }
        else if (str[i] == 0xF4) /* F4 80..8F 80..BF 80..BF */
        {
            if (i + 3 < len) /* Expect a 2nd, 3rd 3th byte */
            {
                if (str[i + 1] < 0x80 || str[i + 1] > 0x8F) {
                    *message = "After a first byte of F4, expecting 2nd byte between 80 and 8F.";
                    *faulty_bytes = 2;
                    return i;
                }
                if (str[i + 2] < 0x80 || str[i + 2] > 0xBF) {
                    *message = "After a first byte of F4, expecting 3rd byte between 80 and BF.";
                    *faulty_bytes = 3;
                    return i;
                }
                if (str[i + 3] < 0x80 || str[i + 3] > 0xBF) {
                    *message = "After a first byte of F4, expecting 4th byte between 80 and BF.";
                    *faulty_bytes = 4;
                    return i;
                }
            } else {
                *message = "After a first byte of F4, expecting three following bytes.";
                *faulty_bytes = 1;
                return i;
            }
            i += 4;
        } else {
            *message = "Expecting bytes in the following ranges: 00..7F C2..F4.";
            *faulty_bytes = 1;
            return i;
        }
    }
    message = NULL;
    return 0;
}

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

static fth_result_t fth_compile(fth_parser *parser, fth_chunk *chunk) {
    char *error = NULL;
    int faulty = 0;
    if (is_utf8(parser->begin, wcslen((wchar_t*)parser->begin), &error, &faulty)) {
        parser->error = error;
        goto BAIL;
    }
    for (;;) {
        parser->current = next_token(parser);
        fth_print_token(&parser->current);
        switch (parser->current.type) {
            case FTH_TOKEN_EOF:
                emit(parser, chunk, FTH_TOKEN_EOF);
            case FTH_TOKEN_ERROR:
                goto BAIL;
            case FTH_TOKEN_ATOM:
                break;
            case FTH_TOKEN_STRING:
                break;
            case FTH_TOKEN_NUMBER:
                emit_number(parser, chunk);
                break;
            case FTH_TOKEN_INTEGER:
                emit_integer(parser, chunk);
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
            case FTH_TOKEN_RIGHT_PAREN:
                break;
            case FTH_TOKEN_LEFT_SQR_PAREN:
                break;
            case FTH_TOKEN_RIGHT_SQR_PAREN:
                break;
            case FTH_TOKEN_LEFT_BRKT_PAREN:
                break;
            case FTH_TOKEN_RIGHT_BRKT_PAREN:
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
        parser->previous = parser->current;
    }
BAIL:
    return parser->error == NULL ? FTH_OK : FTH_COMPILE_ERROR;
}
