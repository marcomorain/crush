#include <stdio.h>

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css


struct tokeniser;
typedef int(*state)(struct tokeniser*);

struct tokeniser {
    FILE* input;
    unsigned current;
    unsigned next;
    state state;
};

enum
{
    TOKEN_IDENT,
    TOKEN_FUNCTION,
    TOKEN_AT_KEYWORD,
    TOKEN_HASH,
    TOKEN_STRING,
    TOKEN_BAD_STRING,
    TOKEN_URL,
    TOKEN_BAD_URL,
    TOKEN_DELIM,
    TOKEN_NUMBER,
    TOKEN_PERCENTAGE,
    TOKEN_DIMENSION,
    TOKEN_UNICODE_RANGE,
    TOKEN_INCLUDE_MATCH,
    TOKEN_DASH_MATCH,
    TOKEN_PREFIX_MATCH,
    TOKEN_SUFFIX_MATCH,
    TOKEN_SUBSTRING_MATCH,
    TOKEN_COLUMN,
    TOKEN_WHITESPACE,
    TOKEN_CDO,
    TOKEN_CDC,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LEFT_SQUARE,
    TOKEN_RIGHT_SQUARE,
    TOKEN_PAREN_LEFT,
    TOKEN_PAREN_RIGHT,
    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY
};

int state_double_quoted_string(struct tokeniser* t)
{
    return 0;
}

int state_single_quoted_string(struct tokeniser* t)
{
    return 0;
}

int state_data(struct tokeniser* t)
{
    switch(t->next)
    {
        case '"':
            t->state = state_double_quoted_string;
            break;

        case '\'':
            t->state = state_single_quoted_string;
            break;

        case '(':
            // skip
            return TOKEN_PAREN_LEFT;

        case ')':
            // skip
            return TOKEN_PAREN_RIGHT;

        case ',':
            // skip
            return TOKEN_COMMA;

        case ':':
            // skip
            return TOKEN_COLON;
            
    }

    return 0;
}

void tokeniser_init(struct tokeniser* t, FILE* input)
{
    t->input = input;
    t->current = 0;
    t->state = state_data;
}

int main(int argc, const char * argv[])
{
    struct tokeniser t;
    tokeniser_init(&t, stdin);

    for (;;)
    {
        t.state(&t);
    }

    return 0;
}

