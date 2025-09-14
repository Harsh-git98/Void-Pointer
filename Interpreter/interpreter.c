#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.txt>\\n", argv[0]);
        return 1;
    }
    char *code = read_file(argv[1]);
    execute_code(code);
    free(code);
    return 0;
}
