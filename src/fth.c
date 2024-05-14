/* fth.c -- https://github.com/takeiteasy/fth
 
 The MIT License (MIT)
 
 Copyright (c) 2024 George Watson
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "fth.h"

void fth_init(fth_state_t *fth, int maxStackSize) {
    if (!(fth->stack.length = maxStackSize))
        fth->stack.length = FTH_DEFAULT_STACK_SIZE;
    assert((fth->stack.entries = malloc(fth->stack.length * sizeof(fth_value_t))));
    fth->stack.head = 0;
}

void fth_destroy(fth_state_t *fth) {
    if (fth->stack.entries)
        free(fth->stack.entries);
    if (fth->lexer.ast.head) {
        fth_entry_t *cursor = fth->lexer.ast.head;
        while (cursor) {
            fth_entry_t *next = cursor->next;
            if (cursor->value.type == FTH_TYPE_STRING)
                free((void*)cursor->value.value.tstring);
            free(cursor);
            cursor = next;
        }
    }
    memset(fth, 0, sizeof(fth_state_t));
}

static fth_value_t fth_value(fth_type_t type, void *value) {
    fth_value_t result = {.type = type};
    switch (type) {
        case FTH_TYPE_INTEGER:;
            fth_int_t *tinteger = (fth_int_t*)value;
            result.value.tinteger = *tinteger;
            break;
        case FTH_TYPE_FLOAT:;
            fth_float_t *tfloat = (fth_float_t*)value;
            result.value.tfloat = *tfloat;
            break;
        case FTH_TYPE_ERROR:
        case FTH_TYPE_STRING:
            result.value.tstring = strdup((const char*)value);
            break;
        default:
            abort(); // invalid type
    }
    return result;
}

static void push_ast_entry(fth_state_t *fth, fth_value_t v) {
    fth_entry_t *entry = malloc(sizeof(fth_entry_t));
    entry->value = v;
    entry->next = NULL;
    
    if (!fth->lexer.ast.head && !fth->lexer.ast.tail)
        fth->lexer.ast.head = fth->lexer.ast.tail = entry;
    else {
        assert(fth->lexer.ast.head && fth->lexer.ast.tail);
        fth->lexer.ast.tail->next = entry;
        fth->lexer.ast.tail = entry;
    }
}

static void free_ast_entry(fth_entry_t **_e) {
    fth_entry_t *e = *_e;
    if (!e || !_e)
        return;
    if (e->value.type == FTH_TYPE_STRING)
        free((void*)e->value.value.tstring);
    free(e);
    _e = NULL;
}

static fth_entry_t* pop_ast_head(fth_state_t *fth, int *empty) {
    fth_entry_t *head = fth->lexer.ast.head;
    if (!head)
        goto BAIL;
    fth->lexer.ast.head = head->next;
BAIL:
    if (empty)
        *empty = head == NULL || head->next == NULL;
    return head;
}

static int check_overflow(fth_state_t *fth, int delta) {
    return fth->lexer.cursor + delta >= fth->lexer.length;
}

static char peek_next_char(fth_state_t *fth) {
    return check_overflow(fth, 1) ? 0 : fth->lexer.str[fth->lexer.cursor + 1];
}

static char peek_char(fth_state_t *fth) {
    return check_overflow(fth, 0) ? 0 : fth->lexer.str[fth->lexer.cursor];
}

static char inc_char(fth_state_t *fth) {
    if (check_overflow(fth, 1))
        fth->lexer.eof = 1;
    char result = peek_char(fth);
    fth->lexer.cursor++;
    return result;
}

static fth_value_t parse_word(fth_state_t *fth, int *word_length) {
    static char word_buffer[FTH_DEFAULT_MAX_WORD_SIZE];
    memset(word_buffer, 0, FTH_DEFAULT_MAX_WORD_SIZE);
    word_buffer[0] = '\0';
    int offset = 0;
    while (!fth->lexer.eof) {
        char c = peek_char(fth);
        if (c == ' ' || c == '\n')
            goto RETURN;
        word_buffer[offset++] = c;
        if (offset >= FTH_DEFAULT_MAX_WORD_SIZE)
            abort(); // symbol too long
        inc_char(fth);
    }
    
RETURN:
    if (!offset || word_buffer[0] == '\0')
        abort();
    if (word_buffer[offset-1] == '\n')
        word_buffer[--offset] = '\0';
    else if (offset < FTH_DEFAULT_MAX_WORD_SIZE)
        word_buffer[offset] = '\0';
    if (word_length)
        *word_length = offset;
    
    const char *tmp = malloc(offset * sizeof(char));
    memcpy((void*)tmp, word_buffer, offset * sizeof(char));
    return fth_value(FTH_TYPE_STRING, (void*)tmp);
}

static fth_value_t parse_number(fth_state_t *fth) {
    int length;
    fth_value_t word = parse_word(fth, &length);
    if (!word.value.tstring || !length)
        abort();
    const char *str = word.value.tstring;
    
    fth_type_t type = FTH_TYPE_INTEGER;
    for (int i = 0; i < length; i++) {
        char c = str[i];
        switch (c) {
            case '.':
                switch (type) {
                    case FTH_TYPE_INTEGER:
                        type = FTH_TYPE_FLOAT;
                        break;
                    case FTH_TYPE_FLOAT:
                        return fth_value(FTH_TYPE_STRING, (void*)str);
                    default:
                        abort(); // unknown error
                }
            case '0' ... '9':
                break;
            default:
                return fth_value(FTH_TYPE_STRING, (void*)str);
        }
    }
    
    switch (type) {
        case FTH_TYPE_INTEGER:;
            fth_int_t tinteger = atoi(str);
            free((void*)str);
            return fth_value(FTH_TYPE_INTEGER, (void*)&tinteger);
        case FTH_TYPE_FLOAT:;
            fth_float_t tfloat = atof(str);
            free((void*)str);
            return fth_value(FTH_TYPE_FLOAT, (void*)&tfloat);
        default:
            abort(); // unknown error
    }
}

static int parse_fth(fth_state_t *fth, const char *str, size_t size) {
    while (!fth->lexer.eof) {
        char next = peek_char(fth);
        switch (next) {
            case ' ':
            case '\r':
            case '\n':
                inc_char(fth);
                break;
            case ';':
                while (!fth->lexer.eof) {
                    char c = inc_char(fth);
                    if (c == '\n')
                        break;
                }
                break;
            case '0' ... '9':;
                fth_value_t tnumber = parse_number(fth);
                switch (tnumber.type) {
                    case FTH_TYPE_INTEGER:
                    case FTH_TYPE_FLOAT:
                    case FTH_TYPE_STRING:
                        push_ast_entry(fth, tnumber);
                        break;
                    default:
                        abort(); // unknown error
                }
                break;
            default:;
                int length = 0;
                fth_value_t word = parse_word(fth, &length);
                push_ast_entry(fth, word);
                break;
        }
    }
    return 1;
}

int fth_parse(fth_state_t *fth, const char *str, size_t size) {
    fth->lexer.str = str;
    fth->lexer.cursor = 0;
    fth->lexer.length = size;
    fth->lexer.eof = 0;
    fth->lexer.ast.head = fth->lexer.ast.tail = NULL;
    return parse_fth(fth, str, size);
}

static int does_file_exist(const char *path) {
    return !access(path, F_OK);
}

static const char* file_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    return !dot || dot == path ? NULL : dot + 1;
}


int fth_parse_file(fth_state_t *fth, const char *path) {
    const char *ext = file_extension(path);
    if (!ext || strlen(ext) != 1 || (ext[0] != 'f' && ext[0] == 'F'))
        abort(); // invalid file type
    if (!does_file_exist(path))
        abort(); // file doesn't exist
    FILE *fh = fopen(path, "rb");
    assert(fh);
    fseek(fh, 0, SEEK_END);
    size_t sz = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    char *data = malloc(sz * sizeof(char));
    fread(data, sz, 1, fh);
    fclose(fh);
    int result = fth_parse(fth, (const char*)data, sz);
    free(data);
    return result;
}

static int push_to_stack(fth_value_t value) {
    switch (value.type) {
        case FTH_TYPE_INTEGER:
            printf("INTEGER: %lu\n", value.value.tinteger);
            break;
        case FTH_TYPE_FLOAT:
            printf("FLOAT: %f\n", value.value.tfloat);
            break;
        case FTH_TYPE_STRING:
            printf("WORD: %s\n", value.value.tstring);
            break;
        case FTH_TYPE_ERROR:
            printf("ERROR: %s\n", value.value.tstring);
            break;
    }
    return 1;
}

static int fth_eval_entry(fth_state_t *fth, fth_entry_t *entry) {
    int result = 1;
    switch (entry->value.type) {
        default:
            abort(); // invalid type, push error then fall-thru
        case FTH_TYPE_ERROR:
            result = 0;
        case FTH_TYPE_INTEGER:
        case FTH_TYPE_FLOAT:
        case FTH_TYPE_STRING:;
            result = push_to_stack(entry->value);
    }
    return result;
}

int fth_eval_step(fth_state_t *fth, int *empty) {
    fth_entry_t* head = pop_ast_head(fth, empty);
    int result = !!head;
    if (head) {
        result = fth_eval_entry(fth, head);
        free_ast_entry(&head);
    }
    return result;
}

int fth_eval(fth_state_t *fth) {
    int empty = 0, error = 0;
    while (!empty && !error)
        error = !fth_eval_step(fth, &empty);
    return !error;
}
