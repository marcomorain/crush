#pragma once
#include <stdio.h>

enum token_type
{
    TOKEN_EOF = 0,
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
    TOKEN_COLON        = ':',
    TOKEN_SEMICOLON    = ';',
    TOKEN_COMMA        = ',',
    TOKEN_LEFT_SQUARE  = '[',
    TOKEN_RIGHT_SQUARE = ']',
    TOKEN_PAREN_LEFT   = '(',
    TOKEN_PAREN_RIGHT  = ')',
    TOKEN_LEFT_CURLY   = '{',
    TOKEN_RIGHT_CURLY  = '}',
};

struct lexer;
struct token;
struct lexer* lexer_init(FILE* input);
struct token* lexer_next(struct lexer* L);
enum token_type token_type(struct token* t);
const char* token_name(int t);
void token_free(struct token* t);

// Test
double token_number(struct token* t);
int token_range_low(struct token* t);
int token_range_high(struct token* t);


// Parse
struct stylesheet;
struct stylesheet* parse_stylesheet(struct lexer* L);
void stylesheet_print(struct stylesheet* ss, FILE* file);
