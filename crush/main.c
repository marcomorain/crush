#include "crush.h"

void test(const char* data) {
    FILE* file = file_with_contents(data);
    struct lexer lexer;
    lexer_init(&lexer, file);
    for (;;){
        struct token* token = lexer_next(&lexer);
        token_print(token);
        if (token->type == TOKEN_NONE) break;
    }
}

int main(int argc, const char * argv[])
{
    test("body { }");

    FILE* input;
    struct lexer L;

    if (argc < 2) {
        input = stdin;
    } else {
        input = fopen(argv[1], "r");
    }

    lexer_init(&L, input);

    L.logging.consumtion = true;
    L.logging.trace = true;

    for (;;)
    {
        struct token* token = lexer_next(&L);
        if (token->type == TOKEN_NONE)
        {
            break;
        }
        token_print(token);
    }

    return 0;
}