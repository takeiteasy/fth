#include "src/fth.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
    fth_vm vm;
    fth_init(&vm);
    if (fth_exec_file(&vm, "test.f") != FTH_OK) {
        printf("ERROR: %s\n", vm.error);
        free(vm.error);
    }
    fth_destroy(&vm);
    return 0;
}
