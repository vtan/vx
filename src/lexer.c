#include "vxc.h"

struct strings strings = {0};
struct lexemes lexemes = {0};

enum current_lexeme_type {
    CUR_NONE,
    CUR_WORD,
    CUR_INT_LITERAL,
};

static void push_lexeme(enum current_lexeme_type*, struct source_location, const char*, uint64_t);

void lex_file(FILE* file) {
    struct source_location location = {
        .line = 1,
        .column = 0,
    };
    struct source_location lexeme_start = {0};

    enum current_lexeme_type current_lexeme_type = CUR_NONE;
    const char* word_start = NULL;
    uint64_t int_literal;
    bool int_hex_mode;

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        ++location.column;

        if (ch == ' ' || ch == '\t') {
            push_lexeme(&current_lexeme_type, lexeme_start, word_start, int_literal);

        } else if (ch == '\n') {
            push_lexeme(&current_lexeme_type, lexeme_start, word_start, int_literal);
            ++location.line;
            location.column = 0;

        } else if (ch < 0x20) {
            compiler_error(&location, "Illegal control character");
        } else if (ch > 0x7F) {
            compiler_error(&location, "Illegal character above 0x7F");

        } else if (ch == '(' || ch == ')') {
            push_lexeme(&current_lexeme_type, lexeme_start, word_start, int_literal);
            VEC_PUSH(lexemes, ((struct lexeme) {
                .type = (ch == '(' ? LEX_PAREN_OPEN : LEX_PAREN_CLOSE),
                .source_location = location,
            }));

        } else {
            if (current_lexeme_type == CUR_NONE) {
                if (ch >= '0' && ch <= '9') {
                    current_lexeme_type = CUR_INT_LITERAL;
                    lexeme_start = location;
                    int_literal = 0;
                    int_hex_mode = false;
                } else {
                    current_lexeme_type = CUR_WORD;
                    lexeme_start = location;
                    word_start = &strings.buf[strings.size];
                }
            }

            switch (current_lexeme_type) {
                case CUR_INT_LITERAL:
                    // TODO: check for overflow
                    if ((ch == 'x' || ch == 'X') && int_literal == 0 && !int_hex_mode) {
                        int_hex_mode = true;

                    } else if (!int_hex_mode && ch >= '0' && ch <= '9') {
                        uint64_t digit = ch - '0';
                        int_literal = 10 * int_literal + digit;

                    } else if (int_hex_mode && ch >= '0' && ch <= '9') {
                        uint64_t digit = ch - '0';
                        int_literal = 16 * int_literal + digit;

                    } else if (int_hex_mode && ch >= 'a' && ch <= 'f') {
                        uint64_t digit = 0xA + ch - 'a';
                        int_literal = 16 * int_literal + digit;

                    } else if (int_hex_mode && ch >= 'A' && ch <= 'F') {
                        uint64_t digit = 0xA + ch - 'A';
                        int_literal = 16 * int_literal + digit;

                    } else if (ch != '_') {
                        compiler_error(&lexeme_start, "Illegal character in int literal");
                    }
                    break;

                case CUR_WORD:
                    VEC_PUSH(strings, ch);
                    break;

                case CUR_NONE:
                    assert(false);
            }
        }
    }
}

void push_lexeme(
    enum current_lexeme_type* type,
    struct source_location lexeme_start,
    const char* word,
    uint64_t int_literal
) {
    switch (*type) {
        case CUR_WORD:
            VEC_PUSH(strings, '\0');
            VEC_PUSH(lexemes, ((struct lexeme) {
                .type = LEX_WORD,
                .source_location = lexeme_start,
                .word = word,
            }));
            *type = CUR_NONE;
            break;

        case CUR_INT_LITERAL:
            VEC_PUSH(lexemes, ((struct lexeme) {
                .type = LEX_INT_LITERAL,
                .source_location = lexeme_start,
                .int_literal = int_literal,
            }));
            *type = CUR_NONE;
            break;

        case CUR_NONE:
            break;
    }
}
