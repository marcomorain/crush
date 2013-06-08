#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css

const static unsigned BUFFER_INIT_MAX = 32;

struct buffer {
    unsigned* data;
    unsigned  capacity;
    unsigned  size;
    unsigned  initial[BUFFER_INIT_MAX];
};

struct buffer* buffer_init(struct buffer* b) {
    // Premature optimization of course.
    b->size     = 0;
    b->data     = b->initial;
    b->capacity = BUFFER_INIT_MAX;
    return b;
}

void buffer_grow(struct buffer* b)
{
    unsigned* data = b->data;
    b->data = malloc(2 * b->capacity * sizeof(unsigned));
    memcpy(b->data, data, b->capacity * sizeof(unsigned));
    b->capacity *= 2;
    if (data != b->initial){
        free(data);
    }

}
struct buffer* buffer_push(struct buffer* b, unsigned c) {
    if (b->size == b->capacity) {
        buffer_grow(b);
    }
    b->data[b->size++] = c;
    return b;
}

struct lexer {
    FILE* input;

    // The last character to have been consumed.
    unsigned current;

    // The first character in the input stream that has not yet been consumed.
    unsigned next;
    unsigned line;
    unsigned column;
    bool integer;
    bool id; // for hash
    
    struct {
        bool consumtion;
        bool trace;
    } logging;
};

char p(char c){
    // TODO: UNICODE.
    return c == '\n' ? '^' : c;
}



enum {
    CHAR_NULL              = 0,
    CHAR_LINE_FEED         = 0xA,
    CHAR_FORM_FEED         = 0xC,
    CHAR_CARRIAGE_RETURN   = 0xD,
    CHAR_QUOTATION_MARK    = 0x22,
    CHAR_NUMBER_SIGN       = 0x23,
    CHAR_PERCENT_SIGN      = 0x25,
    CHAR_APOSTROPHE        = 0x27,
    CHAR_ASTERISK          = 0x2A,
    CHAR_PLUS_SIGN         = 0x2B,
    CHAR_HYPHEN_MINUS      = 0x2D,
    CHAR_FULL_STOP         = 0x2E,
    CHAR_CIRCUMFLEX_ACCENT = 0x5E,
    CHAR_SOLIDUS           = 0x2F,
    CHAR_LESS_THAN         = 0x3C,
    CHAR_GREATER_THAN      = 0x3E,
    CHAR_QUESTION_MARK     = 0x3F,
    CHAR_COMMERCIAL_AT     = 0x40,
    CHAR_LATIN_CAPITAL_E   = 0x45,
    CHAR_REVERSE_SOLIDUS   = 0x5C,
    CHAR_LOW_LINE          = 0x5F,
    CHAR_LATIN_SMALL_E     = 0x65,
    CHAR_VERTICAL_LINE     = 0x7C,
    CHAR_TILDE             = 0x7E,
    CHAR_CONTROL           = 0x80,
    CHAR_REPLACEMENT       = 0xFFFD,
    CHAR_EOF               = EOF,
};

struct token {
    int       type;
    bool      id;
    unsigned  value;

    unsigned* buffer;
    size_t    buffer_size;
    
    double number;
};

static double string_to_number(bool integer) {
    return 0;
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
    if (L->logging.consumtion) {
        printf("Line %d:%d: unconsuming %c (0x%02X)\n", L->line, L->column, p(L->current), L->current);
    }
    ungetc(L->next, L->input);
    L->next = L->current;
    L->current = CHAR_NULL;
}


void lexer_consume(struct lexer* L)
{
    // consume
    L->current = L->next;
    L->next = lexer_preprocess(L->input);

    if (L->logging.consumtion) {
        printf("Line %d:%d: Consuming %c (0x%02X); next is %c (0x%02X)\n",
               L->line, L->column, p(L->current), L->current, p(L->next), L->next);
    }

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

void* mallocf(size_t size){
    void* result = malloc(size);
    if (!result) {
        fprintf(stderr, "Error allocating memory");
        exit(EXIT_FAILURE);
    }
    return result;
}

struct token* token_simple(int type) {
    struct token* t = calloc(1, sizeof(struct token));
    t->type = type;
    return t;
}

struct token* token_delim(unsigned value) {
    struct token* t = token_simple(TOKEN_DELIM);
    t->value = value;
    return t;
}

struct token* token_new(struct lexer* L, int type, const struct buffer* b) {
    struct token* t = token_simple(type);
    t->id = L->id;

    if (b) {
        t->buffer_size = b->size;
        t->buffer      = mallocf(b->size * sizeof(unsigned));
        memcpy(t->buffer, b->data, b->size * sizeof(unsigned));
    }

    switch (type){
        case TOKEN_NUMBER:
        case TOKEN_PERCENTAGE:
        case TOKEN_DIMENSION:
            t->number = string_to_number(L->integer);
            break;

        case TOKEN_DELIM:
            assert(0 && "use delim ctor");
            break;

        default:
            break;
    }

    return t;
}

void token_print(struct token* t) {
    printf("%s ", token_name(t));

    if (t->buffer_size > 0) {
        printf(" value: \"");
        for (int i = 0;i< t->buffer_size; i++){
            printf("%c", p(t->buffer[i]));
        }
        printf("\"");
    }

    switch (t->type){
        case TOKEN_NUMBER:
        case TOKEN_PERCENTAGE:
        case TOKEN_DIMENSION:
            printf(" numeric value: %g", t->number);
            break;

        case TOKEN_HASH:
            printf(" (%s)", (t->id ? "id" :  "unrestricted"));
            break;

        case TOKEN_DELIM:
            printf(" (%c)", t->value);
            break;

        default:
            break;
    }

    printf("\n");
}

struct token* consume_token(struct lexer* L, struct buffer* b);

void lexer_trace(struct lexer* L, const char* state) {
    if (L->logging.trace) {
        printf("%d:%d %s\n", L->line, L->column, state);
    }
}

#define TRACE(L) lexer_trace(L, __FUNCTION__)

// 4.4.4. Check if two characters are a valid escape

// This section describes how to check if two characters are a valid escape. The
// algorithm described here can be called explicitly with two characters, or can
// be called with the input stream itself. In the latter case, the two characters
// in question are the current input character and the next input character, in
// that order.
//
// This algorithm will not consume any additional characters.
//
// If the first character is not U+005D REVERSE SOLIDUS (\), return false.
// Otherwise, if the second character is a newline or EOF character, return false.
// Otherwise, return true.
static bool valid_escape(unsigned first, unsigned second){
    if (first != CHAR_REVERSE_SOLIDUS) return false;
    if (second == CHAR_LINE_FEED) return false;
    if (second == CHAR_EOF) return false; // TODO: check for signed / unsigned issues here.
    return true;
}

static bool lexer_valid_escape(struct lexer* L) {
    return valid_escape(L->current, L->next);
}

unsigned peek(FILE* input) {
    unsigned result = fgetc(input);
    if (ungetc(result, input) == EOF) {
        exit(EXIT_FAILURE);
    }
    return result;
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

static bool char_letter(unsigned c) {
    return isupper(c) || islower(c);
}

static bool char_non_ascii(unsigned c) {
    return c >= CHAR_CONTROL;
}

static bool char_name_start(unsigned c){
    return char_letter(c) || char_non_ascii(c) || c == CHAR_LOW_LINE;
}

static bool char_name(unsigned c){
    return char_name_start(c) || isdigit(c) || c == CHAR_HYPHEN_MINUS;
}

// 4.4.5. Check if three characters would start an identifier
// This section describes how to check if three characters would start an
// identifier. The algorithm described here can be called explicitly with three
// characters, or can be called with the input stream itself. In the latter case,
// the three characters in question are the current input character and the next
// two input characters, in that order.
// This algorithm will not consume any additional characters.
// Look at the first character:
//
// U+002D HYPHEN-MINUS
// If the second character is a name-start character or the second and third
// characters are a valid escape, return true. Otherwise, return false.
//
// name-start character
// Return true.
//
// U+005C REVERSE SOLIDUS (\)
// If the first and second characters are a valid escape, return true.
// Otherwise, return false.

static bool would_start_ident(unsigned first, unsigned second, unsigned third) {
    if (first == CHAR_HYPHEN_MINUS){
        if (char_name_start(second)) return true;
        if (valid_escape(second, third))  return true;
        return false;
    }
    
    if (char_name_start(first)) return true;
    
    if (first == CHAR_REVERSE_SOLIDUS && valid_escape(first, second)) {
        return true;
    }
    
    return false;
}

static void lexer_next_three(struct lexer* L, unsigned r[3])
{
    r[0] = L->next;
    r[1] = fgetc(L->input);
    r[2] = fgetc(L->input);
    ungetc(r[2], L->input);
    ungetc(r[1], L->input);
}

static bool lexer_next_three_are(struct lexer* L, unsigned a, unsigned b, unsigned c) {
    unsigned r[3];
    lexer_next_three(L, r);
    return a == r[0] && b == r[1] && c == r[2];
}

static bool lexer_would_start_ident(struct lexer* L) {
    return would_start_ident(L->current, L->next, peek(L->input));
}

static bool lexer_next_would_start_ident(struct lexer* L) {
    unsigned r[3];
    lexer_next_three(L, r);
    return would_start_ident(r[0], r[1], r[2]);
}

// 4.4.6. Check if three characters would start a number

// This section describes how to check if three characters would start a number.
// The algorithm described here can be called explicitly with three characters,
// or can be called with the input stream itself.
// In the latter case, the three characters in question are the current input
// character and the next two input characters, in that order.
//
// This algorithm will not consume any additional characters.
//
static bool starts_with_number(unsigned a, unsigned b, unsigned c)
{
    // Look at the first character:
    switch (a) {
        case CHAR_PLUS_SIGN:
        case CHAR_HYPHEN_MINUS:
            // If the second character is a digit, return true.
            if (isdigit(b)){
                return true;
            }

            // Otherwise, if the second character is a U+002E FULL STOP (.) and
            // the third character is a digit, return true.
            if (b == CHAR_FULL_STOP && isdigit(c)){
                return true;
            }
            
            return false;
            
        case CHAR_FULL_STOP:
            // If the second character is a digit, return true. Otherwise, return false.
            return isdigit(b);
            
        default:
            // digit: Return true.
            // anything else: Return false.
            return isdigit(a);
    }
}

static bool lexer_starts_with_number(struct lexer* L) {
    return starts_with_number(L->current, L->next, peek(L->input));
}

unsigned char hex_to_byte(unsigned hex_char){
    hex_char = toupper(hex_char);
    return hex_char > '9' ? hex_char - 'A' + 10 : hex_char - '0';
}

// 4.4.1. Consume an escaped character
//
// This section describes how to consume an escaped character. It assumes that
// the U+005C REVERSE SOLIDUS (\) has already been consumed and that the next
// input character has already been verified to not be a newline or EOF. It will
// return a character.
//
// Consume the next input character.
//
// hex digit:
// Consume as many hex digits as possible, but no more than 5. Note that this
// means 1-6 hex digits have been consumed in total. If the next input character
// is whitespace, consume it as well. Interpret the hex digits as a hexadecimal
// number. If this number is zero, or is greater than the maximum allowed
// codepoint, return U+FFFD REPLACEMENT CHARACTER (). Otherwise, return the
// character with that codepoint.
//
// anything else:
// Return the current input character.

unsigned lexer_consume_escape(struct lexer* L) {
    assert(L->current == CHAR_REVERSE_SOLIDUS);
    lexer_consume(L);

    // not hex
    if (!ishexnumber(L->current)) {
        return L->current;
    }

    // is hex
    unsigned result = hex_to_byte(L->current);
    for (int i=0; i<5; i++){
        if (ishexnumber(L->next)){
            lexer_consume(L);
            unsigned n = hex_to_byte(L->current);
            result = (result << 2) | n;
        }
    }
    return result;
}

struct token* state_ident(struct lexer* L, struct buffer* b) {
    TRACE(L);
    lexer_consume(L);

    if (char_name(L->current)) {
        buffer_push(b, L->current);
        return state_ident(L, b);
    }

    if (L->current == CHAR_REVERSE_SOLIDUS) {
        // If the input stream starts with a valid escape, consume an escaped
        // character.
        // Append the returned character to the <input>’s value.
        // Remain in this state.
        if (lexer_valid_escape(L)) {
            buffer_push(b, lexer_consume_escape(L));
            return state_ident(L, b);
        }

        // Otherwise, emit the <ident>. Switch to the data state. Reconsume the
        // current input character.
        lexer_recomsume(L);
        return token_new(L, TOKEN_IDENT, b);
    }

    // U+0028 LEFT PARENTHESIS (()
    // If the <ident>’s value is an ASCII case-insensitive match for "url", switch to the url state.
    // Otherwise, emit a <function> token with its value set to the <ident>’s value. Switch to the data state.
    if (L->current == '(') {
        // TODO: check for URL.
        token_new(L, TOKEN_FUNCTION, b);
    }

    // anything else
    // Emit the <ident>. Switch to the data state. Reconsume the current input character.
    lexer_recomsume(L);
    return token_new(L, TOKEN_IDENT, b);
}


// 4.3.11. Consume a name
//
// This section describes how to consume a name from a stream of characters.
// It returns a string containing the largest name that can be formed from
// adjacent characters in the stream, starting from the first.
//
// This algorithm does not do the verification of the first few characters that
// are necessary to ensure the returned characters would constitute an 〈ident〉.
// If that is the intended use, ensure that the stream starts with an identifier
// before calling this algorithm.
//
// Let result initially be an empty string.
//
// Repeatedly consume the next input character from the stream:
//
// name character:
//   Append the character to result.
// the stream starts with a valid escape:
//   Consume an escaped character. Append the returned character to result.
// anything else:
//   Return result.
void consume_name(struct lexer* L, struct buffer* b) {
    TRACE(L);
    for (;;) {
        lexer_consume(L);
        if (char_name(L->current)) {
            buffer_push(b, L->current);
        } else if (lexer_valid_escape(L)) {
            buffer_push(b, lexer_consume_escape(L));
        } else {
            return;
        }
    }
}

struct token* state_number_end(struct lexer* L, struct buffer* b)
{
    TRACE(L);
    lexer_consume(L);
    
    switch (L->current){
        case CHAR_PERCENT_SIGN:
            return token_new(L, TOKEN_PERCENTAGE, b);
            
        case CHAR_HYPHEN_MINUS:
            // If the input stream starts with an identifier, create a
            // <dimension> with its representation set to the <number>’s
            // representation, its value set to the <number>’s value, its type
            // flag set to the 〈number〉’s type flag, and a unit initially set
            // to the current input character. Switch to the dimension state.
            if (lexer_would_start_ident(L)){
                // todo  <- value
                // todo  <- dimention
                // todo: <- unit

                struct buffer dimension;
                consume_name(L, buffer_init(&dimension));
                // todo: dimension
                return token_new(L, TOKEN_DIMENSION, b);
            }
            goto state_number_end_default;
        
        case CHAR_REVERSE_SOLIDUS:
            // If the input stream starts with a valid escape, consume an escaped
            // character. Create a <dimension> with its representation set to the
            // <number>’s representation, its value set to the 〈number〉’s value,
            // its type flag set to the 〈number〉’s type flag, and a unit initially
            // set to the returned character. Switch to the dimension state.
            if (lexer_valid_escape(L)){
                // todo: dimenion
                // check buffer here
                return token_new(L, TOKEN_DIMENSION, b); // todo: < number value
            } else
                goto state_number_end_default;
            
        default:
            // Otherwise, emit the 〈number〉. Switch to the data state. Reconsume
            // the current input character.
state_number_end_default:
            lexer_recomsume(L);
            return token_new(L, TOKEN_NUMBER, b);

    }
    
}


// 4.3.9. Number-rest state
//
// U+0045 LATIN CAPITAL LETTER E (E)
// U+0065 LATIN SMALL LETTER E (e)
// If the next input character is a digit, or the next 2 input characters are U+002B
// PLUS SIGN (+) or U+002D HYPHEN-MINUS (-) followed by a digit, consume them.
// Append U+0065 LATIN SMALL LETTER E (e) and the consumed characters to the
// <number>’s representation. Switch to the sci-notation state.
// Otherwise, switch to the number-end state. Reconsume the current input character.
struct token* state_number_rest(struct lexer* L, struct buffer* b) {
    TRACE(L);
    lexer_consume(L);
    
    switch (L->current) {
        case CHAR_FULL_STOP:
        {
            //  U+002E FULL STOP (.)
            // If the <number’s type flag is currently "integer" and the next
            // input character is a digit, consume it. Append U+002E FULL STOP (.)
            // followed by the digit to the <number>’s representation. Set the
            // <number>’s type flag to "number". Remain in this state.
            if (L->integer && isdigit(L->next)) {
                buffer_push(b, L->current);
                lexer_consume(L);
                buffer_push(b, L->current);
                L->integer = false;
                return state_number_rest(L, b);
            }
            // Otherwise, set the <number>’s value to the number produced by
            // interpreting the <number>’s representation as a base-10 number and
            // emit it. Switch to the data state. Reconsume the current input
            // character.
            lexer_recomsume(L);
            return token_new(L, TOKEN_NUMBER, b);
        }
            
        case CHAR_LATIN_CAPITAL_E:
        case CHAR_LATIN_SMALL_E:
            // If the next input character is a digit, or the next 2 input
            // characters are U+002B PLUS SIGN (+) or U+002D HYPHEN-MINUS (-)
            // followed by a digit, consume them. Append U+0065 LATIN SMALL
            // LETTER E (e) and the consumed characters to the 〈number〉’s
            // representation. Switch to the sci-notation state.
            if (isdigit(L->next)) {
                lexer_consume(L);
                buffer_push(b, CHAR_LATIN_SMALL_E);
                buffer_push(b, L->current);
                // TODO
            }
            // Otherwise, switch to the number-end state. Reconsume the current
            // input character.
            lexer_recomsume(L);
            return state_number_end(L, b);
            
        default:
            if (isdigit(L->current)) {
                // digit:
                // Append the current input character to the <number>’s representation. Remain in
                // this state.
                buffer_push(b, L->current);
                return state_number_rest(L, b);
            } else {
                // Set the <number>’s value to the number produced by
                // interpreting the <number>’s representation as a base-10
                // number. Switch to the number-end state. Reconsume the current
                // input character.
                // TODO: parse number as base 10.
                lexer_recomsume(L);
                return state_number_end(L, b);
            }
    }
    
    assert(0 && "end of number state");
    return NULL;
}

struct token* state_number(struct lexer* L, struct buffer* b)
{
    TRACE(L);
    lexer_consume(L);
    L->integer = true;
    
    switch(L->current){
        case CHAR_PLUS_SIGN:
        case CHAR_HYPHEN_MINUS:
            buffer_push(b, L->current);
            if (L->next == CHAR_FULL_STOP) {
                L->integer = false;
                lexer_consume(L);
                buffer_push(b, L->current);
            }
            lexer_consume(L);
            goto number;
            
        case CHAR_FULL_STOP:
            L->integer = false;
            buffer_push(b, L->current);
            lexer_consume(L);
            goto number;
            
        default:
            // Reaching this state indicates an error either
            // in the spec or the implementation.
            assert(isdigit(L->current));
number:
            buffer_push(b, L->current);
            return state_number_rest(L, b);
    }
    
}


// 4.3.6. At-keyword state
//
// When this state is first entered, create an 〈at-keyword〉 token with its value
// initially set to the empty string.
//
// Consume the next input character.
//
struct token* state_at_keyword(struct lexer* L, struct buffer* b)
{
    TRACE(L);
    lexer_consume(L);

    // name character:
    // Append the current input character to the 〈at-keyword〉’s value. Remain
    // in this state.
    if (char_name(L->current)){
        buffer_push(b, L->current);
        return state_at_keyword(L, b);
    }

    // U+005C REVERSE SOLIDUS (\):
    if (L->current == CHAR_REVERSE_SOLIDUS) {
        // If the input stream starts with a valid escape, consume an escaped
        // character Append the returned character to the 〈at-keyword〉’s value.
        // Remain in this state.
        if (lexer_valid_escape(L)) {
            buffer_push(b, lexer_consume_escape(L));
            return state_at_keyword(L, b);
        }
        // Otherwise, emit the 〈at-keyword〉. Switch to the data state. Reconsume
        // the current input character.
        lexer_recomsume(L);
        return token_new(L, TOKEN_AT_KEYWORD, b);
        
    }
    
    // anything else:
    // Emit the 〈at-keyword〉. Switch to the data state.
    // Reconsume the current input character.
    // If this state emits an 〈at-keyword〉 whose value is the empty string, it's
    // a spec or implementation error. The data validation performed in the data
    // state should have guaranteed a non-empty value.
    lexer_recomsume(L);
    return token_new(L, TOKEN_AT_KEYWORD, b);
}

struct token* consume_unicode_range(struct lexer* L){
    assert(0 && "not implemented");
    return token_new(L, TOKEN_UNICODE_RANGE, 0);
}


// 4.3.4. Consume a string token
//
// This section describes how to consume a string token from a stream of
// characters. It returns either a 〈string〉 or 〈bad-string〉.
//
// This algorithm must be called with an ending character, which denotes the character that ends the string.
//
// Initially create a 〈string〉 with its value set to the empty string.
// Repeatedly consume the next input character from the stream:
//
// ending character:
// EOF:
// Return the 〈string〉.
//
// newline:
// This is a parse error. Create a 〈bad-string〉 and return it.
//
// U+005C REVERSE SOLIDUS (\)
// If the stream starts with a valid escape, consume an escaped character and
// append the returned character to the 〈string〉’s value.
// Otherwise, if the next input character is a newline, consume it.
//
// Otherwise, this is a parse error. Create a 〈bad-string〉 and return it.
//
// anything else:
// Append the current input character to the 〈string〉’s value.
struct token* consume_string_token(struct lexer* L, struct buffer* b, unsigned ending)
{
    lexer_consume(L);
    TRACE(L);

    if (L->current == CHAR_EOF || L->current == ending) {
        return token_new(L, TOKEN_STRING, b);
    }

    if (L->current == CHAR_LINE_FEED) {
        return token_new(L, TOKEN_BAD_STRING, b);
    }

    if (L->current == CHAR_REVERSE_SOLIDUS){
        if (lexer_valid_escape(L)){
            buffer_push(b, lexer_consume_escape(L));
            return consume_string_token(L, b, ending);
        }

        if (L->next == CHAR_LINE_FEED){
            lexer_consume(L);
            return consume_string_token(L, b, ending);
        }

        return token_new(L, TOKEN_BAD_STRING, b);
    }

    buffer_push(b, L->current);
    return consume_string_token(L, b, ending);
}

struct token* consume_token(struct lexer* L, struct buffer* b)
{
    TRACE(L);
    lexer_consume(L);

    switch(L->current) {
        // A newline, U+0009 CHARACTER TABULATION, or U+0020 SPACE.
        case CHAR_LINE_FEED:
        case '\t':
        case ' ':
            while(whitespace(L->next)){
                lexer_consume(L);
            }
            return token_new(L, TOKEN_WHITESPACE, NULL);
            
        case CHAR_QUOTATION_MARK:
            return consume_string_token(L, b, CHAR_QUOTATION_MARK);
            
        case CHAR_NUMBER_SIGN:
            // If the next input character is a name character or the next two
            // input characters are a valid escape, If the next three input
            // characters would start an identifier, set the 〈hash〉 token's type
            // flag to "id". Switch to the hash state.
            // Otherwise, emit a 〈delim〉 token with its value set to the current
            // input character. Remain in this state.
            if (char_name(L->next) || valid_escape(L->next, peek(L->input))) {
                L->id = lexer_next_would_start_ident(L);
                struct buffer buffer;
                consume_name(L, buffer_init(&buffer));
                return token_new(L, TOKEN_HASH, &buffer);
            }
            
            return token_delim(L->current);
            
        case '$':
            if (L->next == '=') {
                lexer_consume(L);
                return token_simple(TOKEN_SUFFIX_MATCH);
            }
            return token_delim(L->current);

        case CHAR_APOSTROPHE:
            return consume_string_token(L, b, CHAR_APOSTROPHE);
            break;

        case '(':
            return token_simple(TOKEN_PAREN_LEFT);

        case ')':
            return token_simple(TOKEN_PAREN_RIGHT);

        case CHAR_ASTERISK:
            if (L->next == '=') {
                lexer_consume(L);
                return token_simple(TOKEN_SUBSTRING_MATCH);
            }
            return token_delim(L->current);

        case '+':
            if (lexer_starts_with_number(L)){
                lexer_recomsume(L);
                return state_number(L, b);
            }
            return token_delim(L->current);

        case ',':
            return token_simple(TOKEN_COMMA);

        case CHAR_HYPHEN_MINUS:
            if (lexer_starts_with_number(L)){
                lexer_recomsume(L);
                return state_number(L, b);
            }

            if (lexer_would_start_ident(L)){
                lexer_recomsume(L);
                return state_ident(L, b);
            }

            if (L->next == CHAR_HYPHEN_MINUS && peek(L->input) == CHAR_GREATER_THAN) {
                lexer_consume(L);
                lexer_consume(L);
                return token_simple(TOKEN_CDC);
            }

            return token_delim(L->current);

        case CHAR_FULL_STOP:
            // If the input stream starts with a number, reconsume the current
            // input character and switch to the number state.
            if (lexer_starts_with_number(L)) {
                lexer_recomsume(L);
                return state_number(L, b);
            }
            // Otherwise, emit a <delim> token with its value set to the current
            // input character. Remain in this state.
            return token_delim(L->current);

        case CHAR_SOLIDUS:
            if (L->next == CHAR_ASTERISK) {
                // Skip a /* comment */
                // todo: comment token? important comments.
                printf("comment skipping: ");
                lexer_consume(L);
                for (;;) {
                    printf("%c", L->next);
                    lexer_consume(L);
                    if (L->current == CHAR_EOF ||
                        (L->current == CHAR_ASTERISK && L->next == CHAR_SOLIDUS)){
                        lexer_consume(L);
                        break;
                    }
                }
                printf("\n");
                // go again.
                return consume_token(L, b);
            }
            return token_delim(L->current);

        case ':':
            return token_simple(TOKEN_COLON);
            
        case ';':
            return token_simple(TOKEN_SEMICOLON);

        case CHAR_LESS_THAN:
            if (lexer_next_three_are(L, '!', CHAR_HYPHEN_MINUS, CHAR_HYPHEN_MINUS)){
                return token_simple(TOKEN_CDO);
            }
            return token_delim(L->current);
            
        case '{':
            return token_simple(TOKEN_LEFT_CURLY);
            
        case '}':
            return token_simple(TOKEN_RIGHT_CURLY);
            
        case CHAR_COMMERCIAL_AT:
            // If the next 3 input characters would start an identifier, switch
            // to the at-keyword state.
            if (lexer_next_would_start_ident(L)){
                return state_at_keyword(L, b);
            }
            
            // Otherwise, emit a 〈delim〉 token with its value set to the current
            // input character. Remain in this state.
            return token_delim(L->current);
            
            
        case '[':
            return token_simple(TOKEN_LEFT_SQUARE);
            
        case ']':
            return token_simple(TOKEN_RIGHT_SQUARE);

        case CHAR_CIRCUMFLEX_ACCENT:
            if (L->next == '='){
                lexer_consume(L);
                return token_simple(TOKEN_PREFIX_MATCH);
            }
             return token_delim(L->current);

        case CHAR_VERTICAL_LINE:
            if (L->next == '=') {
                lexer_consume(L);
                return token_simple(TOKEN_DASH_MATCH);
            }
            if (L->next == CHAR_VERTICAL_LINE) {
                lexer_consume(L);
                return token_simple(TOKEN_COLUMN);
            }
            return token_simple(TOKEN_DELIM);

        case CHAR_TILDE:
            if (L->next == '=') {
                lexer_consume(L);
                return token_simple(TOKEN_INCLUDE_MATCH);
            }
            return token_simple(TOKEN_DELIM);

        case CHAR_LATIN_CAPITAL_E:
        case CHAR_LATIN_SMALL_E:
            if (L->next == CHAR_PLUS_SIGN){
                unsigned second = peek(L->input);
                if(ishexnumber(second) || second == CHAR_QUESTION_MARK){
                    // consume the CHAR_PLUS_SIGN
                    lexer_consume(L);
                    return consume_unicode_range(L);
                }
            }
            lexer_recomsume(L);
            return state_ident(L, b);


        case CHAR_EOF:
            return token_simple(TOKEN_NONE);

        default:
            if (char_name_start(L->current)) {
                lexer_recomsume(L);
                return state_ident(L, b);
            }
            
            if (isdigit(L->current)){
                lexer_recomsume(L);
                return state_number(L, b);
            }

            return token_simple(TOKEN_DELIM);
            
    }
    printf("Unexpected input at line %d:%d %c (0x%02X)\n", L->line, L->column, p(L->current), L->current);
    return token_simple(TOKEN_NONE);
}

void lexer_init(struct lexer* L, FILE* input)
{
    memset(L, 0, sizeof(struct lexer));
    L->input   = input;
    L->next    = fgetc(input);
    L->line    = 1;
}

struct token* lexer_next(struct lexer* L)
{
    struct buffer buffer;
    return consume_token(L, buffer_init(&buffer));
}

int main(int argc, const char * argv[])
{
    FILE* input;
    struct lexer L;
    
    if (argc < 2) {
        input = stdin;
    } else {
        input = fopen(argv[1], "r");
    }

    lexer_init(&L, input);
    
    //L.logging.consumtion = true;
    //L.logging.trace = true;

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

