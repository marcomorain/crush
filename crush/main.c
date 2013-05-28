#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>
//#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css


struct lexer;
typedef struct token*(*state)(struct lexer*);

struct lexer {
    FILE* input;
    unsigned current;
    unsigned next;
    state state;
    unsigned line;
    unsigned column;
    
};

char p(char c){
    // TODO: UNICODE.
    return c == '\n' ? '^' : c;
}


enum {
    CHAR_NULL            = 0,
    CHAR_LINE_FEED       = 0xA,
    CHAR_FORM_FEED       = 0xC,
    CHAR_CARRIAGE_RETURN = 0xD,
    CHAR_HYPHEN_MINUS    = 0x2D,
    CHAR_REPLACEMENT     = 0xFFFD,
};

struct token {
    int       type;
    unsigned* buffer;
};

struct token* token_new(int type) {
    struct token* t = calloc(1, sizeof(struct token));
    t->type = type;
    return t;
}

/*
 3.2.1. Preprocessing the input stream

 The input stream consists of the characters pushed into it as the input byte
 stream is decoded.

 Before sending the input stream to the tokenizer, implementations must make the
 following character substitutions:

 - Replace any U+000D CARRIAGE RETURN (CR) characters, U+000C FORM FEED (FF)
   characters, or pairs of U+000D CARRIAGE RETURN (CR) followed by U+000A LINE
   FEED (LF) by a single U+000A LINE FEED (LF) character.
 - Replace any U+0000 NULL characters with U+FFFD REPLACEMENT CHARACTER.
 */
static unsigned lexer_preprocess(FILE* input) {
    unsigned next = fgetc(input);

    if (next == CHAR_NULL) {
        return CHAR_REPLACEMENT;
    }

    if (next == CHAR_CARRIAGE_RETURN) {
        next = fgetc(input);
        if (next != CHAR_LINE_FEED) {
            ungetc(next, input);
        }
        return CHAR_LINE_FEED;
    }

    return next;
}

void lexer_recomsume(struct lexer* L)
{
    printf("Line %d:%d: unconsuming %c (0x%02X)\n", L->line, L->column, p(L->current), L->current);
    ungetc(L->next, L->input);
    L->next = L->current;
    L->current = CHAR_NULL;
}


void lexer_consume(struct lexer* L)
{
    // consume
    L->current = L->next;
    L->next = lexer_preprocess(L->input);

    printf("Line %d:%d: Consuming %c (0x%02X); next is %c (0x%02X)\n",
           L->line, L->column, p(L->current), L->current, p(L->next), L->next);

    L->column++;
    if (L->current == CHAR_LINE_FEED){
        L->line++;
        L->column = 0;
    }
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

const char* token_name(struct token* t){
    switch (t->type){
#define NAME(X) case (X): return #X;
        NAME(TOKEN_NONE);
        NAME(TOKEN_IDENT);
        NAME(TOKEN_FUNCTION);
        NAME(TOKEN_AT_KEYWORD);
        NAME(TOKEN_HASH);
        NAME(TOKEN_STRING);
        NAME(TOKEN_BAD_STRING);
        NAME(TOKEN_URL);
        NAME(TOKEN_BAD_URL);
        NAME(TOKEN_DELIM);
        NAME(TOKEN_NUMBER);
        NAME(TOKEN_PERCENTAGE);
        NAME(TOKEN_DIMENSION);
        NAME(TOKEN_UNICODE_RANGE);
        NAME(TOKEN_INCLUDE_MATCH);
        NAME(TOKEN_DASH_MATCH);
        NAME(TOKEN_PREFIX_MATCH);
        NAME(TOKEN_SUFFIX_MATCH);
        NAME(TOKEN_SUBSTRING_MATCH);
        NAME(TOKEN_COLUMN);
        NAME(TOKEN_WHITESPACE);
        NAME(TOKEN_CDO);
        NAME(TOKEN_CDC);
        NAME(TOKEN_COLON);
        NAME(TOKEN_SEMICOLON);
        NAME(TOKEN_COMMA);
        NAME(TOKEN_LEFT_SQUARE);
        NAME(TOKEN_RIGHT_SQUARE);
        NAME(TOKEN_PAREN_LEFT);
        NAME(TOKEN_PAREN_RIGHT);
        NAME(TOKEN_LEFT_CURLY);
        NAME(TOKEN_RIGHT_CURLY);
#undef NAME
    }
    return "";
}

struct token* state_double_quoted_string(struct lexer* t)
{
    return 0;
}

struct token* state_single_quoted_string(struct lexer* t)
{
    return 0;
}

struct token* state_data(struct lexer* L);

void lexer_trace(struct lexer* L, const char* state){
    printf("%d:%d %s\n", L->line, L->column, state);
}

#define TRACE(L) lexer_trace(L, __FUNCTION__)

struct token* state_comment(struct lexer* L)
{
state_comment_again:

    TRACE(L);

    lexer_consume(L);

    switch (L->current){

        case '*':
        {
            if (L->next == '/'){
                lexer_consume(L);
                L->state = state_data;
                return L->state(L);
            } else {
                // stay in this state
                goto state_comment_again;
            }
        }

        case EOF:
        {
            // lexer_recomsume() // TODO
            return TOKEN_NONE;
        }

        default:{
            goto state_comment_again;
        }
    }

    return TOKEN_NONE;
}

static bool whitespace(unsigned c) {
    switch (c) {
        case CHAR_LINE_FEED:
        case '\t':
        case ' ':
            return true;
        default:
            return false;
    }
}

static bool name_start_character(unsigned c){
    return isalpha(c) || (!isascii(c));
}

static bool name_character(unsigned c){
    return name_start_character(c) || isdigit(c) || c == CHAR_HYPHEN_MINUS;
}


struct token* state_ident(struct lexer* L) {
    TRACE(L);
    lexer_consume(L);

    if (name_character(L->current)){
        printf("TODO: append %c to buffer\n", L->current);
        return L->state(L);
    }

    if (L->current == '\\'){
        // TODO deal with this.
        /*
        U+005C REVERSE SOLIDUS (\)
        If the input stream starts with a valid escape, consume an escaped character. Append the returned character to the 〈input〉’s value. Remain in this state.
        Otherwise, emit the 〈ident〉. Switch to the data state. Reconsume the current input character.
         */
        return TOKEN_NONE;
    }

    /*
     U+0028 LEFT PARENTHESIS (()
     If the 〈ident〉’s value is an ASCII case-insensitive match for "url", switch to the url state.
     Otherwise, emit a 〈function〉 token with its value set to the 〈ident〉’s value. Switch to the data state.
     */

    if (L->current == '(') {
        // TODO: check for URL.
        L->state = state_data;
        token_new(TOKEN_FUNCTION); // TODO: value <- ident
    }

    // anything else
    // Emit the 〈ident〉. Switch to the data state. Reconsume the current input character.
    L->state = state_data;
    lexer_recomsume(L);
    return token_new(TOKEN_IDENT); // TODO:
}

struct token* state_data(struct lexer* L)
{
    TRACE(L);
    lexer_consume(L);


    switch(L->current)
    {
        // A newline, U+0009 CHARACTER TABULATION, or U+0020 SPACE.
        case CHAR_LINE_FEED:
        case '\t':
        case ' ':
            while(whitespace(L->next)){
                lexer_consume(L);
            }
            return token_new(TOKEN_WHITESPACE);

        case '$':
            if (L->next == '=') {
                lexer_consume(L);
                return token_new(TOKEN_SUFFIX_MATCH);
            }
            return token_new(TOKEN_DELIM); // todo: value <- current char
            
        case '"':
            L->state = state_double_quoted_string;
            break;

        case '/':
            if (L->next == '*')
            {
                lexer_consume(L);
                L->state = state_comment;
                return L->state(L);
                break;
            }
            else
            {
                return token_new(TOKEN_DELIM); // todo: value <= solidus (/)
            }

        case '\'':
            L->state = state_single_quoted_string;
            break;

        case ',':
            return token_new(TOKEN_COMMA);

        case ':':
            return token_new(TOKEN_COLON);
            
        case ';':
            return token_new(TOKEN_SEMICOLON);

        case '(':
            return token_new(TOKEN_PAREN_LEFT);

        case ')':
            return token_new(TOKEN_PAREN_RIGHT);
            
        case '{':
            return token_new(TOKEN_LEFT_CURLY);
            
        case '}':
            return token_new(TOKEN_RIGHT_CURLY);
            
        case '[':
            return token_new(TOKEN_LEFT_SQUARE);
            
        case ']':
            return token_new(TOKEN_RIGHT_SQUARE);
            
        default:
            if (name_start_character(L->current)) {
                lexer_recomsume(L);
                L->state = state_ident;
                return L->state(L);
            }
            break;
            
    }
    printf("Unexpected input %c (0x%02X)\n", p(L->current), L->current);
    return token_new(TOKEN_NONE);
}

void lexer_init(struct lexer* L, FILE* input)
{
    L->input   = input;
    L->current = 0;
    L->next    = fgetc(input);
    L->state   = state_data;
    L->column  = 0;
    L->line    = 1;
}

struct token* lexer_next(struct lexer* L)
{
    struct token* t = L->state(L);
    printf("> Emited token %s\n", token_name(t));
    return t;
}

int main(int argc, const char * argv[])
{
    struct lexer L;
    lexer_init(&L, stdin);

    for (;;)
    {
        struct token* token = lexer_next(&L);
        if (token->type == TOKEN_NONE)
        {
            break;
        }
    }

    return 0;
}

