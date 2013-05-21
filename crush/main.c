#include <stdio.h>

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css


struct lexer;
typedef int(*state)(struct lexer*);

struct lexer {
    FILE* input;
    unsigned current;
    unsigned next;
    state state;
};

void lexer_consume(struct lexer* L)
{
    // consume
    L->current = L->next;
    L->next = fgetc(L->input);
    printf("Consuming '%c'; next is '%c'\n", L->current, L->next);
}

enum
{
    TOKEN_NONE = 0,
    
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

int state_double_quoted_string(struct lexer* t)
{
    return 0;
}

int state_single_quoted_string(struct lexer* t)
{
    return 0;
}

int state_data(struct lexer* L)
{
    lexer_consume(L);
    
state_data_again:
    switch(L->next)
    {
        // A newline, U+0009 CHARACTER TABULATION, or U+0020 SPACE.
        case '\n':
        case '\t':
        case ' ':
            lexer_consume(L);
            goto state_data_again;
            
        case '"':
            lexer_consume(L);
            L->state = state_double_quoted_string;
            break;

        case '\'':
            L->state = state_single_quoted_string;
            break;

        case ',':
            return TOKEN_COMMA;

        case ':':
            return TOKEN_COLON;
            
        case ';':
            return TOKEN_SEMICOLON;

        case '(':
            return TOKEN_PAREN_LEFT;

        case ')':
            return TOKEN_PAREN_RIGHT;
            
        case '{':
            return TOKEN_LEFT_CURLY;
            
        case '}':
            return TOKEN_RIGHT_CURLY;
            
        case '[':
            return TOKEN_LEFT_SQUARE;
            
        case ']':
            return TOKEN_RIGHT_SQUARE;
            
        default:
            break;
            
    }
    printf("Unexpected intput %c\n", L->next);
    return TOKEN_NONE;
}

void lexer_init(struct lexer* L, FILE* input)
{
    L->input   = input;
    L->current = 0;
    L->next    = 0;
    L->state   = state_data;
}

int lexer_next(struct lexer* L)
{
    int token = L->state(L);
    printf("Emiited token %d\n", token);
    return token;
}

int main(int argc, const char * argv[])
{
    struct lexer L;
    lexer_init(&L, stdin);

    for (;;)
    {
        int token = lexer_next(&L);
        if (token == TOKEN_NONE)
        {
            break;
        }
    }

    return 0;
}

