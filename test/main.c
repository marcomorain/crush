#include "crush.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

static int passes = 0;
static int fails   = 0;

static int fail(const char* format, ...) {
    fails++;
    va_list arg_list;
    va_start (arg_list, format);
    vfprintf(stderr, format, arg_list);
    va_end(arg_list);
    return EXIT_FAILURE;
}

static FILE* file_with_contents(const char* data){
    FILE* file = tmpfile();
    fwrite(data, strlen(data), 1, file);
    rewind(file);
    return file;
}

int test_number(const char* repr, double value){
    FILE* file = file_with_contents(repr);
    struct lexer* lexer = lexer_init(file);
    struct token* token = lexer_next(lexer);

    if (token_type(token) != TOKEN_NUMBER){
        return fail("Token was %s, not a number\n", token_name(token_type(token)));
    }

    double actual = token_number(token);



    token = lexer_next(lexer);

    if (token_type(token) != TOKEN_NONE){
        return fail("Token was %s, expected end of input\n", token_name(token_type(token)));
    }

    if (fabs(actual - value) > 0.0000001){
        return fail("Expected %g but got %g\n", value, token_number(token));
    }

    passes++;
    return EXIT_SUCCESS;
}

int test(const char* data, const int* tokens) {

    FILE* file = file_with_contents(data);
    struct lexer* lexer = lexer_init(file);
    for (;;) {
        struct token* token = lexer_next(lexer);
        int found = token_type(token);
        if (found == TOKEN_NONE) break;

        if (found != *tokens){
            fail("Error in \"%s\" expected %s but got %s\n", data, token_name(*tokens), token_name(found));
            token_print(stderr, token);
            return 0;
        }
        tokens++;
    }
    fclose(file);

    if (*tokens != TOKEN_NONE) {
        fail("Error token none at the end of the input %s\n", token_name(*tokens));
        return 0;
    }

    fprintf(stdout, "pass => %s\n", data);
    passes++;
    return 1;
}

int main(int argc, const char * argv[])
{
    test_number("123E+123", 123E+123);
    test_number("1.0", 1.0);
    test_number("1.01", 1.01);
    test_number("-8", -8);
    test_number("+8", +8);
    test_number("8e1", 8E1);


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
