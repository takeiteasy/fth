#include "src/fth.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
    fth_vm vm;
    fth_init(&vm);
    fth_result_t result = fth_exec_file(&vm, "test.f");
    switch (result) {
        case FTH_OK:
            printf("OK\n");
            break;
        case FTH_COMPILE_ERROR:
            printf("COMPILATION ERROR: %s\n", vm.error);
            break;
        case FTH_RUNTIME_ERROR:
            printf("RUNTIME ERROR: %s\n", vm.error);
            break;
    }
    if (vm.error)
        free(vm.error);
    fth_destroy(&vm);
    return 0;
}
