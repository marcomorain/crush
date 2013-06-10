#include "crush.h"
#include <stdio.h>
#include <string.h>

static int passes = 0;
static int fails   = 0;

static FILE* file_with_contents(const char* data){
    FILE* file = tmpfile();
    fwrite(data, strlen(data), 1, file);
    rewind(file);
    return file;
}

int test(const char* data, const int* tokens) {

    FILE* file = file_with_contents(data);
    struct lexer* lexer = lexer_init(file);
    for (;;) {
        struct token* token = lexer_next(lexer);
        int found = token_type(token);
        if (found == TOKEN_NONE) break;

        if (found != *tokens){
            fprintf(stderr, "Error expected token on type %d but got %s", found, token_name(token));
            fails++;
            return 0;
        }
        tokens++;
    }
    fclose(file);
    passes++;
    return 1;
}

int main(int argc, const char * argv[])
{
    int a[] = {TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_LEFT_CURLY, TOKEN_WHITESPACE, TOKEN_RIGHT_CURLY};
    test("body { }", a);
    printf("passed: %d failed: %d", passes, fails);
    return 0;
}