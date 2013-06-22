#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include "crush.h"

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css

const static unsigned BUFFER_INIT_MAX = 32;

// Code point
typedef int cp;

struct buffer {
    size_t capacity;
    size_t size;
    cp* data;
    cp  initial[BUFFER_INIT_MAX];
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
    cp* data = b->data;
    b->data = malloc(2 * b->capacity * sizeof(cp));
    memcpy(b->data, data, b->capacity * sizeof(cp));
    b->capacity *= 2;
    if (data != b->initial){
        free(data);
    }

}

static bool buffer_logging = false;

struct buffer* buffer_push(struct buffer* b, cp c) {
    if (b->size == b->capacity) {
        buffer_grow(b);
    }
    b->data[b->size++] = c;
    if (buffer_logging){
        printf("Pushing '%c' input buffer. [", c);
        for (int i=0; i<b->size; i++){
            printf("%c", b->data[i]);
        }
        printf("] size %ld\n", b->size);
    }
    return b;
}

struct lexer {
    FILE* input;

    // The last character to have been consumed.
    cp current;

    // The first character in the input stream that has not yet been consumed.
    cp next;
    cp line;
    cp column;
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
    CHAR_TABULATION        = 0x9,
    CHAR_LINE_FEED         = 0xA,
    CHAR_FORM_FEED         = 0xC,
    CHAR_CARRIAGE_RETURN   = 0xD,
    CHAR_SPACE             = 0x20,
    CHAR_EXCLAMATION_MARK  = 0x21,
    CHAR_QUOTATION_MARK    = 0x22,
    CHAR_NUMBER_SIGN       = 0x23,
    CHAR_DOLLAR_SIGN       = 0x24,
    CHAR_PERCENT_SIGN      = 0x25,
    CHAR_APOSTROPHE        = 0x27,
    CHAR_LEFT_PARENTHESIS  = 0x28,
    CHAR_RIGHT_PARENTHESIS = 0x29,
    CHAR_ASTERISK          = 0x2A,
    CHAR_PLUS_SIGN         = 0x2B,
    CHAR_COMMA             = 0x2C,
    CHAR_HYPHEN_MINUS      = 0x2D,
    CHAR_FULL_STOP         = 0x2E,
    CHAR_CIRCUMFLEX_ACCENT = 0x5E,
    CHAR_SOLIDUS           = 0x2F,
    CHAR_COLON             = 0x3A,
    CHAR_SEMICOLON         = 0x3B,
    CHAR_LESS_THAN         = 0x3C,
    CHAR_EQUALS_SIGN       = 0x3D,
    CHAR_GREATER_THAN      = 0x3E,
    CHAR_QUESTION_MARK     = 0x3F,
    CHAR_COMMERCIAL_AT     = 0x40,
    CHAR_LATIN_CAPITAL_E   = 0x45,
    CHAR_LEFT_SQUARE       = 0x5B,
    CHAR_REVERSE_SOLIDUS   = 0x5C,
    CHAR_RIGHT_SQUARE      = 0x5D,
    CHAR_LOW_LINE          = 0x5F,
    CHAR_LATIN_SMALL_E     = 0x65,
    CHAR_LEFT_CURLY        = 0x7B,
    CHAR_VERTICAL_LINE     = 0x7C,
    CHAR_RIGHT_CURLY       = 0x7D,
    CHAR_TILDE             = 0x7E,
    CHAR_CONTROL           = 0x80,
    CHAR_REPLACEMENT       = 0xFFFD,
    CHAR_EOF               = EOF,
    CHAR_MAX_CODE_POINT    = 0x10FFFF,
};

struct token {
    enum token_type type;

    union {

        struct  {
            bool integer; // type is integer or number
            double value; // Numeric value
        } number;

        struct {
            bool id; // hash type is ID?
        } hash;

        struct {
            cp value;
        } delim;

    } value;

    cp* buffer;
    size_t buffer_size;
};

void token_free(struct token* t) {
    free(t->buffer);
    free(t);
}

enum token_type token_type(struct token* t) {
    return t->type;
}


static double string_to_number(bool integer) {
    return 0;
}

void ungetcf(int c, FILE* stream){
    if (ungetc(c, stream) != c){
        fprintf(stderr, "Error putting 0x%X back into input stream.\n", c);
        exit(EXIT_FAILURE);
    }
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
static cp lexer_preprocess(FILE* input) {
    cp next = fgetc(input);

    if (next == CHAR_NULL) {
        return CHAR_REPLACEMENT;
    }

    if (next == CHAR_CARRIAGE_RETURN) {
        next = fgetc(input);
        if (next != CHAR_LINE_FEED) {
            ungetcf(next, input);
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
    ungetcf(L->next, L->input);
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

const char* token_name(int t){
    switch (t){
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

struct token* token_delim(cp value) {
    struct token* t = token_simple(TOKEN_DELIM);
    t->value.delim.value = value;
    return t;
}

struct token* token_new(struct lexer* L, int type, const struct buffer* b) {
    struct token* t = token_simple(type);

    t->value.hash.id = L->id;

    if (b) {
        t->buffer_size = b->size;
        t->buffer      = mallocf(b->size * sizeof(cp));
        memcpy(t->buffer, b->data, b->size * sizeof(cp));
    }

    switch (type){
        case TOKEN_NUMBER:
        case TOKEN_PERCENTAGE:
        case TOKEN_DIMENSION:
            t->value.number.value = string_to_number(L->integer);
            break;

        case TOKEN_DELIM:
            assert(0 && "use delim ctor");
            break;

        default:
            break;
    }

    return t;
}

static void buffer_print(FILE* file, struct token* t){
    for (int i = 0;i< t->buffer_size; i++){
        fprintf(file, "%c", p(t->buffer[i]));
    }
}

static int depth = 0;

void token_print(FILE* file, struct token* t) {

    fprintf(file, "[%s => ", token_name(token_type(t)));

    switch (t->type) {

        case TOKEN_NONE:
            break;

        case TOKEN_URL:
            fprintf(file, "url(");
            break;

        case TOKEN_STRING:
            fprintf(file, "'");
            buffer_print(file, t);
            fprintf(file, "'");
            break;


        case TOKEN_FUNCTION:
            fprintf(file, "Function: ");
            buffer_print(file, t);
            break;

        case TOKEN_AT_KEYWORD:
            fprintf(file, "@");
            buffer_print(file, t);
            break;

        case TOKEN_LEFT_CURLY:
            depth++;
            fprintf(file, "{");
            break;

        case TOKEN_RIGHT_CURLY:
            depth--;
            fprintf(file, "}");
            break;

        case TOKEN_COLON:
            fprintf(file, "colon -> : ");
            break;

        case TOKEN_SEMICOLON:
            fprintf(file, "semicolon -> ;");
            break;

        case TOKEN_IDENT:
            buffer_print(stdout, t);
            break;

        case TOKEN_NUMBER:
        case TOKEN_PERCENTAGE:
        case TOKEN_DIMENSION:
            //fprintf(file, "%g", t->number);
            buffer_print(file, t);
            break;

        case TOKEN_HASH:
            fprintf(file, "#");
            buffer_print(file, t);
            fprintf(file, " (%s)", (t->value.hash.id ? "id" :  "unrestricted"));
            break;

        case TOKEN_DELIM:
            fprintf(file, "Delim: %c", t->value.delim.value);
            break;
    }


    fprintf(file, "]\n");
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
static bool valid_escape(cp first, cp second){
    if (first != CHAR_REVERSE_SOLIDUS) return false;
    if (second == CHAR_LINE_FEED) return false;
    if (second == CHAR_EOF) return false;
    return true;
}

static bool lexer_valid_escape(struct lexer* L) {
    return valid_escape(L->current, L->next);
}

cp peek(FILE* input) {
    cp result = fgetc(input);
    ungetcf(result, input);
    return result;
}

static bool whitespace(cp c) {
    switch (c) {
        case CHAR_LINE_FEED:
        case CHAR_TABULATION:
        case CHAR_SPACE:
            return true;
        default:
            return false;
    }
}

static bool char_letter(cp c) {
    return isupper(c) || islower(c);
}

static bool char_non_ascii(int c) {
    return c >= CHAR_CONTROL;
}

static bool char_name_start(cp c){
    return char_letter(c) || char_non_ascii(c) || c == CHAR_LOW_LINE;
}

static bool char_name(cp c){
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

static bool would_start_ident(cp first, cp second, cp third) {
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

static void lexer_next_three(struct lexer* L, cp r[3])
{
    r[0] = L->next;
    r[1] = fgetc(L->input);
    r[2] = fgetc(L->input);
    ungetcf(r[2], L->input);
    ungetcf(r[1], L->input);
}

static bool lexer_next_three_are(struct lexer* L, cp a, cp b, cp c) {
    cp r[3];
    lexer_next_three(L, r);
    return a == r[0] && b == r[1] && c == r[2];
}

static bool lexer_would_start_ident(struct lexer* L) {
    return would_start_ident(L->current, L->next, peek(L->input));
}

static bool lexer_next_would_start_ident(struct lexer* L) {
    cp r[3];
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
static bool starts_with_number(cp a, cp b, cp c)
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

static unsigned char hex_to_byte(cp hex_char){
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

static cp lexer_consume_escape(struct lexer* L) {
    assert(L->current == CHAR_REVERSE_SOLIDUS);
    lexer_consume(L);

    // not hex
    if (!ishexnumber(L->current)) {
        return L->current;
    }

    // is hex
    cp result = hex_to_byte(L->current);
    for (int i=0; i<5; i++){
        if (ishexnumber(L->next)){
            lexer_consume(L);
            cp n = hex_to_byte(L->current);
            result = (result << 2) | n;
        }
    }
    return result;
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
            // TODO: This seems to break the draft spec:
            // http://dev.w3.org/csswg/css-syntax/#consume-a-token
            lexer_recomsume(L);
            return;
        }
    }
}

// TODO: make all functions in this file static.
struct token* consume_string_token(struct lexer* L, struct buffer* b, cp ending);

static void consume_bad_url_remnants()
{
    assert(0);
}

static bool non_printable(cp c){
    // A character between U+0000 NULL and U+0008 BACKSPACE,
    if (c <= 8) return true;
    // LINE TABULATION)
    if (c == 0xB) return true;

    // U+000E SHIFT OUT and U+001F INFORMATION SEPARATOR ONE
    if (c >= 0xE && c <= 0x1F) return true;
    if (c == 0x7F) return true;
    return false;
}

struct token* consume_url(struct lexer* L) {
    TRACE(L);

    lexer_consume(L);

    while (whitespace(L->current)){
        lexer_consume(L);
    }

    struct buffer url;
    buffer_init(&url);

    switch (L->next){
        case CHAR_EOF:
        // TODO: should bad URL have data associated with it?
        return token_simple(TOKEN_BAD_URL);

        case CHAR_APOSTROPHE:
        case CHAR_QUOTATION_MARK:
        {
            struct token* href = consume_string_token(L, &url, L->next);
            href->type = TOKEN_BAD_URL;
            if (href->type == TOKEN_BAD_STRING){
                // TODO: consume the remnants of a bad url,
                consume_bad_url_remnants();
                return href;
            }

            // TODO: test for whitespace after the string.
            while (whitespace(L->next)){
                lexer_consume(L);
            }
            if (L->next == CHAR_RIGHT_PARENTHESIS || L->next == CHAR_EOF) {
                lexer_consume(L);
                href->type = TOKEN_URL;
                return href;
            }

            consume_bad_url_remnants();
            return href;
        }

        default:
            break;
    }

    for (;;) {
        lexer_consume(L);

        switch (L->current) {
            case CHAR_EOF:
            case CHAR_RIGHT_PARENTHESIS:
                return token_new(L, TOKEN_URL, &url);
                break;

            // whitespace
            case CHAR_LINE_FEED:
            case CHAR_TABULATION:
            case CHAR_SPACE:
                while(whitespace(L->next)){
                    lexer_consume(L);
                }
                // This seems to duplicate the first case in this switch
                if (L->next == CHAR_EOF || L->next == CHAR_RIGHT_PARENTHESIS){
                    lexer_consume(L);
                    return token_new(L, TOKEN_URL, &url);
                } else goto url_parse_error;

            case CHAR_QUOTATION_MARK:
            case CHAR_APOSTROPHE:
            case CHAR_LEFT_PARENTHESIS:
                goto url_parse_error;
                // todo: check non-printanble

            case CHAR_REVERSE_SOLIDUS:
                if (lexer_valid_escape(L)){
                    buffer_push(&url, lexer_consume_escape(L));
                }
                else goto url_parse_error;

            default:
                if (non_printable(L->current)) goto url_parse_error;
                buffer_push(&url, L->current);
                break;
        }
    }

url_parse_error:
    non_printable(L->current);
    consume_bad_url_remnants();
    return token_new(L, TOKEN_BAD_URL, &url);
}

struct token* consume_ident_like(struct lexer* L, struct buffer* b) {
    TRACE(L);

    consume_name(L, b);

    if (L->next != CHAR_LEFT_PARENTHESIS) {
        return token_new(L, TOKEN_IDENT, b);
    }

    if (b->size == 3 &&
        tolower(b->data[0]) == 'u' &&
        tolower(b->data[1]) == 'r' &&
        tolower(b->data[2]) == 'l' &&
        L->next == CHAR_LEFT_PARENTHESIS) {

        lexer_consume(L);
        return consume_url(L);
    }

    return token_new(L, TOKEN_FUNCTION, b);
}

struct token* state_number_end(struct lexer* L, struct buffer* b)
{
    TRACE(L);
    lexer_consume(L);

    switch (L->current) {
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

struct token* consume_numeric(struct lexer* L, struct buffer* b)
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
struct token* consume_string_token(struct lexer* L, struct buffer* b, cp ending)
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
        case CHAR_TABULATION:
        case CHAR_SPACE:
            while(whitespace(L->next)){
                lexer_consume(L);
            }
            return token_new(L, TOKEN_WHITESPACE, NULL);

            // Fall through for the two string types
        case CHAR_QUOTATION_MARK:
        case CHAR_APOSTROPHE:
            return consume_string_token(L, b, L->current);

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

        case CHAR_DOLLAR_SIGN:
            if (L->next == CHAR_EQUALS_SIGN) {
                lexer_consume(L);
                return token_simple(TOKEN_SUFFIX_MATCH);
            }
            return token_delim(L->current);

        case CHAR_LEFT_PARENTHESIS:
            return token_simple(TOKEN_PAREN_LEFT);

        case CHAR_RIGHT_PARENTHESIS:
            return token_simple(TOKEN_PAREN_RIGHT);

        case CHAR_ASTERISK:
            if (L->next == CHAR_EQUALS_SIGN) {
                lexer_consume(L);
                return token_simple(TOKEN_SUBSTRING_MATCH);
            }
            return token_delim(L->current);

        case CHAR_PLUS_SIGN:
            if (lexer_starts_with_number(L)){
                lexer_recomsume(L);
                return consume_numeric(L, b);
            }
            return token_delim(L->current);

        case CHAR_COMMA:
            return token_simple(TOKEN_COMMA);

        case CHAR_HYPHEN_MINUS:
            if (lexer_starts_with_number(L)){
                lexer_recomsume(L);
                return consume_numeric(L, b);
            }

            if (lexer_would_start_ident(L)){
                lexer_recomsume(L);
                return consume_ident_like(L, b);
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
                return consume_numeric(L, b);
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

        case CHAR_COLON:
            return token_simple(TOKEN_COLON);

        case CHAR_SEMICOLON:
            return token_simple(TOKEN_SEMICOLON);

        case CHAR_LESS_THAN:
            if (lexer_next_three_are(L, CHAR_EXCLAMATION_MARK, CHAR_HYPHEN_MINUS, CHAR_HYPHEN_MINUS)){
                return token_simple(TOKEN_CDO);
            }
            return token_delim(L->current);

        case CHAR_LEFT_CURLY:
            return token_simple(TOKEN_LEFT_CURLY);

        case CHAR_RIGHT_CURLY:
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


        case CHAR_LEFT_SQUARE:
            return token_simple(TOKEN_LEFT_SQUARE);

        case CHAR_RIGHT_SQUARE:
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
            if (L->next == CHAR_EQUALS_SIGN) {
                lexer_consume(L);
                return token_simple(TOKEN_INCLUDE_MATCH);
            }
            return token_simple(TOKEN_DELIM);

        case CHAR_LATIN_CAPITAL_E:
        case CHAR_LATIN_SMALL_E:
            if (L->next == CHAR_PLUS_SIGN){
                cp second = peek(L->input);
                if(ishexnumber(second) || second == CHAR_QUESTION_MARK){
                    // consume the CHAR_PLUS_SIGN
                    lexer_consume(L);
                    return consume_unicode_range(L);
                }
            }
            lexer_recomsume(L);
            return consume_ident_like(L, b);


        case CHAR_EOF:
            return token_simple(TOKEN_NONE);

        default:
            if (char_name_start(L->current)) {
                lexer_recomsume(L);
                return consume_ident_like(L, b);
            }

            if (isdigit(L->current)){
                lexer_recomsume(L);
                return consume_numeric(L, b);
            }

            return token_simple(TOKEN_DELIM);

    }
    printf("Unexpected input at line %d:%d %c (0x%02X)\n", L->line, L->column, p(L->current), L->current);
    return token_simple(TOKEN_NONE);
}

struct lexer* lexer_init(FILE* input)
{
    struct lexer* L = calloc(sizeof(struct lexer), 1);
    L->input   = input;
    L->next    = fgetc(input);
    L->line    = 1;
    L->logging.consumtion = true;
    L->logging.trace = true;
    buffer_logging = true;
    return L;
}

struct token* lexer_next(struct lexer* L)
{
    struct buffer buffer;
    return consume_token(L, buffer_init(&buffer));
}
