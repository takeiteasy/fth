//
//  chunk.inl
//  fth
//
//  Created by George Watson on 06/01/2025.
//

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
