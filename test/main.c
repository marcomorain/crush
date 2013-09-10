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
    printf("FILE: %s\n", data);
    FILE* file = tmpfile();
    fwrite(data, strlen(data), 1, file);
    rewind(file);
    return file;
}

int test_range(const char* repr, int low, int high) {
    FILE* file = file_with_contents(repr);
    struct lexer* lexer = lexer_init(file);
    struct token* token = lexer_next(lexer);

    if (token_type(token) != TOKEN_UNICODE_RANGE){
        return fail("Token was %s, not a range\n", token_name(token_type(token)));
    }

    int actual_low = token_range_low(token);
    int actual_high = token_range_high(token);

    struct token* then = lexer_next(lexer);
    if (token_type(then) != TOKEN_EOF){
        return fail("Token was %s, expected end of input\n", token_name(token_type(then)));
    }

    if (actual_low != low) {
        return fail("Expected range low %d but got %d\n", low, actual_low);
    }

    if (actual_high != high) {
        return fail("Expected range high %d but got %d\n", high, actual_high);
    }

    passes++;
    return EXIT_SUCCESS;
}

int test_number_to(const char* repr, double value, double delta) {
    FILE* file = file_with_contents(repr);
    struct lexer* lexer = lexer_init(file);
    struct token* token = lexer_next(lexer);

    if (token_type(token) != TOKEN_NUMBER){
        return fail("Token was %s, not a number\n", token_name(token_type(token)));
    }

    double actual = token_number(token);

    struct token* then = lexer_next(lexer);
    if (token_type(then) != TOKEN_EOF){
        return fail("Token was %s, expected end of input\n", token_name(token_type(then)));
    }

    if (fabs(actual - value) > delta) {
        return fail("Expected %g but got %g\n", value, token_number(token));
    }

    passes++;
    return EXIT_SUCCESS;
}

int test_number(const char* repr, double value) {
    return test_number_to(repr, value, 0.0000001);
}

int test(const char* data, const int* tokens) {

    FILE* file = file_with_contents(data);
    struct lexer* lexer = lexer_init(file);
    for (;;) {
        struct token* token = lexer_next(lexer);
        int found = token_type(token);
        if (found == TOKEN_EOF) break;

        if (found != *tokens){
            fail("Error in \"%s\" expected %s but got %s\n", data, token_name(*tokens), token_name(found));
            token_free(token);
            return 0;
        }
        tokens++;
        token_free(token);
    }
    fclose(file);

    if (*tokens != TOKEN_EOF) {
        fail("Error token none at the end of the input %s\n", token_name(*tokens));
        return 0;
    }

    fprintf(stdout, "pass => %s\n", data);
    passes++;
    return 1;
}

void numbers() {
    test_number("123E+10", 123E+10);
    test_number("1.0", 1.0);
    test_number("1.01", 1.01);
    test_number("-8", -8);
    test_number("+8", +8);
    test_number("8e1", 8E1);
}

void tokens() {

    int aa[] = {TOKEN_IDENT, TOKEN_EOF};
    test("div.head", aa);

    int ab[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_LEFT_CURLY, TOKEN_RIGHT_CURLY, TOKEN_EOF};
    test("div.head table { }", ab);

    int a[] = {TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_LEFT_CURLY, TOKEN_WHITESPACE, TOKEN_RIGHT_CURLY, TOKEN_EOF};
    test("body { }", a);

    int b[] = {TOKEN_STRING, TOKEN_EOF};
    test("'test'", b);

    int c[] = {TOKEN_STRING, TOKEN_EOF};
    test("\"apple\"", c);

    int d[] = {TOKEN_SUFFIX_MATCH, TOKEN_EOF};
    test("$=", d);

    int e[] = {TOKEN_DELIM, TOKEN_NUMBER, TOKEN_EOF};
    test("$1", e);

    int f[] = { TOKEN_SUBSTRING_MATCH, TOKEN_EOF };
    test("*=", f);

    int g[] = { TOKEN_DELIM, TOKEN_NUMBER, TOKEN_EOF };
    test("*9", g);

    int h[] = {TOKEN_NUMBER, TOKEN_EOF };
    test("+1", h);

    int i[] = {TOKEN_DELIM, TOKEN_IDENT, TOKEN_EOF };
    test("+a", i);

    int j[] = {TOKEN_IDENT, TOKEN_COLON, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_EOF };
    test("color: orange", j);

    int k[] = {TOKEN_URL, TOKEN_EOF };
    test("url(http://example.com)", k);

    int l[] = {TOKEN_URL, TOKEN_EOF };
    test("url(\"http://example.com\")", l);

    int m[] = {TOKEN_UNICODE_RANGE, TOKEN_NUMBER, TOKEN_EOF };
    test("U+1234567", m);

    int n[] = {TOKEN_UNICODE_RANGE, TOKEN_NUMBER, TOKEN_EOF };
    test("U+123456-1234567", n);

    int o[] = { TOKEN_DELIM, TOKEN_NUMBER, TOKEN_INCLUDE_MATCH, TOKEN_EOF };
    test("~.2~=", o);

    int p[] = { TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_EOF };
    test("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", p);

    int q[] = { TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_EOF };
    test("abc\n\rabc\nabc\rabc", q);

    int r[] = {TOKEN_DASH_MATCH, TOKEN_DASH_MATCH, TOKEN_COLUMN, TOKEN_DELIM, TOKEN_EOF};
    test("|=|=|||", r);

    int s[] = {TOKEN_NUMBER, TOKEN_COMMA, TOKEN_NUMBER, TOKEN_EOF};
    test("1,456", s);

    int t[] = {TOKEN_HASH, TOKEN_WHITESPACE, TOKEN_DELIM, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_EOF};
    test("#hashtag # hashtag", t);

    int u[] = {TOKEN_DELIM, TOKEN_IDENT, TOKEN_DELIM, TOKEN_EOF};
    test("/* a comment *//*another*//slash/", u);

    int v[] = {TOKEN_AT_KEYWORD, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_DELIM, TOKEN_WHITESPACE, TOKEN_IDENT, TOKEN_EOF};
    test("@keyword foo @ bar", v);

    int w[] = {TOKEN_IDENT, TOKEN_WHITESPACE, TOKEN_LEFT_CURLY, TOKEN_WHITESPACE,
        TOKEN_IDENT, TOKEN_COLON, TOKEN_WHITESPACE, TOKEN_PERCENTAGE,
        TOKEN_WHITESPACE, TOKEN_RIGHT_CURLY, TOKEN_EOF};
    test("foo { width: 100% }", w);

    int x[] = {TOKEN_FUNCTION, TOKEN_NUMBER, TOKEN_COMMA, TOKEN_NUMBER, TOKEN_COMMA, TOKEN_NUMBER, TOKEN_PAREN_RIGHT, TOKEN_EOF};
    test("rgb(1,1,1)", x);

    int y[] = {TOKEN_IDENT, TOKEN_COLON, TOKEN_WHITESPACE, TOKEN_STRING, TOKEN_EOF };
    test("content: \"\\2193\"", y);

    int z[] = {TOKEN_CDO, TOKEN_CDC, TOKEN_CDO, TOKEN_CDC, TOKEN_EOF};
    test("<!-- --> <!-- -->", z);


}

void ranges() {
    test_range("U+1", 1, 1);
    test_range("U+?", 0, 0xF);
    test_range("U+15-17", 0x15, 0x17);
}

void* parse(const char* data) {
    FILE* file = file_with_contents(data);
    struct lexer* lexer = lexer_init(file);
    struct stylesheet* ss = parse_stylesheet(lexer);
    stylesheet_print(ss, stdout);
    return ss;
}

int main(int argc, const char * argv[])
{
    (void)argc;
    (void)argv;
    ranges();
    numbers();
    tokens();

    parse("@media all { /* c */ a img { color: inherit; } /* d */ } th, td { /* ns 4 */ font-family: sans-serif; }");
    //parse("foo { }");
    //parse("foo { width: 100% }");
    //parse("foo { color: rgb(255, 200, 0); }");

    printf("passed: %d failed: %d\n", passes, fails);
    return 0;
}
