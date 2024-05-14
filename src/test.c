#include "fth.h"

int main(int argc, const char *argv[]) {
    fth_state_t fth;
    fth_init(&fth, 32);
    int presult = fth_parse_file(&fth, "/Users/george/git/ace/test.f");
    assert(presult);
    int eresult = fth_eval(&fth);
    assert(eresult);
    fth_destroy(&fth);
    return 0;
}
