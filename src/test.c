#define FTH_IMPL
#include "fth.h"

int main(int argc, const char *argv[]) {
    fth_chunk chunk;
    chunk_init(&chunk);
    chunk_write(&chunk, FTH_RETURN, 0);
    chunk_disassemble(&chunk, "test");
    
    chunk_free(&chunk);
    return 0;
}
