#include "crush.h"
#include <stdio.h>

int main(int argc, const char * argv[])
{
    FILE* input;

    if (argc < 2) {
        input = stdin;
    } else {
        input = fopen(argv[1], "r");
    }

    struct lexer* L = lexer_init(input);
    struct stylesheet* ss = parse_stylesheet(L);
    stylesheet_print(ss, stdout);

    return 0;
}