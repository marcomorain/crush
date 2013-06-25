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
            fprintf(stderr, "Error in \"%s\" expected %s but got %s\n", data, token_name(*tokens), token_name(found));
            token_print(stderr, token);
            fails++;
            return 0;
        }
        tokens++;
    }
    fclose(file);

    if (*tokens != TOKEN_NONE){
        fprintf(stderr, "Error token none at the end of the input %s\n", token_name(*tokens));
        fails++;
        return 0;
    }

    fprintf(stdout, "pass => %s\n", data);
    passes++;
    return 1;
}

int main(int argc, const char * argv[])
{
    int a[] = {TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_LEFT_CURLY, TOKEN_WHITESPACE, TOKEN_RIGHT_CURLY, TOKEN_NONE};
    test("body { }", a);

    int b[] = {TOKEN_STRING, TOKEN_NONE};
    test("'test'", b);

    int c[] = {TOKEN_STRING, TOKEN_NONE};
    test("\"apple\"", c);

    int d[] = {TOKEN_SUFFIX_MATCH, TOKEN_NONE};
    test("$=", d);

    int e[] = {TOKEN_DELIM, TOKEN_NUMBER, TOKEN_NONE};
    test("$1", e);

    int f[] = { TOKEN_SUBSTRING_MATCH, TOKEN_NONE };
    test("*=", f);

    int g[] = { TOKEN_DELIM, TOKEN_NUMBER, TOKEN_NONE };
    test("*9", g);

    int h[] = {TOKEN_NUMBER, TOKEN_NONE };
    test("+1", h);

    int i[] = {TOKEN_DELIM, TOKEN_IDENT, TOKEN_NONE };
    test("+a", i);

    int j[] = {TOKEN_IDENT, TOKEN_COLON, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_NONE };
    test("color: orange", j);

    int k[] = {TOKEN_URL, TOKEN_NONE };
    test("url(http://example.com)", k);

    int l[] = {TOKEN_URL, TOKEN_NONE };
    test("url(\"http://example.com\")", l);

    printf("passed: %d failed: %d\n", passes, fails);
    return 0;
}
