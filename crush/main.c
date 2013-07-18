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

    for (;;)
    {
        struct token* token = lexer_next(L);
        if (token_type(token) == TOKEN_EOF) {
            break;
        }
        token_print(stdout, token);
    }

    return 0;
}