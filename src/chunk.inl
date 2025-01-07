//
//  chunk.inl
//  fth
//
//  Created by George Watson on 06/01/2025.
//

typedef enum {
    FTH_OP_RETURN,
    FTH_OP_CONSTANT,
    FTH_OP_CONSTANT_LONG,
    FTH_OP_CLEAR,
    FTH_OP_PERIOD,
    FTH_OP_POP,
    FTH_OP_PUSH,
    FTH_OP_DUMP,
    FTH_OP_DUMP_RSTACK
} fth_vm_op;

typedef struct {
    int offset, line;
} fth_chunk_linestart;

struct fth_chunk {
    uint8_t *data;
    fth_value *constants;
    fth_chunk_linestart *lines;
};

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
        case FTH_OP_PERIOD:
            return simple_instruction("OP_PERIOD", offset);
        case FTH_OP_DUMP:
            return simple_instruction("OP_DUMP_STACK", offset);
        case FTH_OP_DUMP_RSTACK:
            return simple_instruction("OP_DUMP_RSTACK", offset);
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
