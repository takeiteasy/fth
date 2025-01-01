#define FTH_IMPL
#include "fth.h"

int main(int argc, const char *argv[]) {
    fth_chunk chunk;
    chunk_init(&chunk);
    chunk_write_constant(&chunk, 123, 0);
    chunk_write_constant(&chunk, 3.4, 1);
    chunk_write(&chunk, FTH_OP_ADD, 2);
    chunk_write_constant(&chunk, 5.6, 3);
    chunk_write(&chunk, FTH_OP_DIV, 4);
    chunk_write(&chunk, FTH_OP_NEGATE, 5);
    chunk_write(&chunk, FTH_OP_RETURN, 6);
    chunk_disassemble(&chunk, "test");
    chunk_free(&chunk);
    return 0;
}
