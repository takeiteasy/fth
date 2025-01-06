#include "src/fth.h"

int main(int argc, const char *argv[]) {
    fth_vm vm;
    fth_init(&vm);
    fth_exec_file(&vm, "test.f");
    fth_destroy(&vm);
    return 0;
}
