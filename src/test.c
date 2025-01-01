#define FTH_IMPL
#include "fth.h"

static void repl(void) {
    fth_vm vm;
    fth_init(&vm);
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        switch (fth_interpret(&vm, line)) {
            case FTH_OK:
                break;
            case FTH_COMPILE_ERROR:
                break;
            case FTH_RUNTIME_ERROR:
                break;
        }
    }
    fth_deinit(&vm);
}

int main(int argc, const char *argv[]) {
    switch (argc) {
        case 1:
            repl();
            break;
        case 2:;
            fth_vm vm;
            fth_init(&vm);
            fth_chunk chunk;
            vm.chunk = &chunk;
            switch(fth_run_file(&vm, argv[1])) {
                case FTH_OK:
                    printf(" OK");
                    break;
                case FTH_COMPILE_ERROR:
                    fprintf(stderr, "COMPILATION ERROR!\n");
                    break;
                case FTH_RUNTIME_ERROR:
                    fprintf(stderr, "RUNTIME ERROR!\n");
                    break;
            }
            fth_deinit(&vm);
            break;
        default:
            fprintf(stderr, "usage: fth [path]\n");
            exit(1);
    }
    return 0;
}
