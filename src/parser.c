#include "vxc.h"

struct ast ast = {0};

struct lexemes lexemes = {
    .size = 13,
    .buf = {
        { .type = LEX_PAREN_OPEN },
        { .type = LEX_WORD, .word = "let" },
        { .type = LEX_WORD, .word = "a" },
        { .type = LEX_PAREN_OPEN },
        { .type = LEX_WORD, .word = "+" },
        { .type = LEX_PAREN_OPEN },
        { .type = LEX_WORD, .word = "-" },
        { .type = LEX_INT_LITERAL, .int_literal = 0x2000 },
        { .type = LEX_INT_LITERAL, .int_literal = 0xD00 },
        { .type = LEX_PAREN_CLOSE },
        { .type = LEX_INT_LITERAL, .int_literal = 0x37 },
        { .type = LEX_PAREN_CLOSE },
        { .type = LEX_PAREN_CLOSE },
    },
};

struct state {
    size_t next_lexeme_index;
};

struct ast_stmt_node* parse_statement(struct state* state);
struct ast_expr_node* parse_expression(struct state* state);

struct lexeme* advance_lexeme(struct state* state);
struct lexeme* expect_paren_open(struct state* state);
void expect_paren_close(struct state* state);
struct lexeme* expect_word(struct state* state);

struct ast_stmt_node* parse_lexemes() {
    struct state state = {
        .next_lexeme_index = 0
    };
    struct ast_stmt_node* root_node = parse_statement(&state);
    if (state.next_lexeme_index < lexemes.size) {
        compiler_error(&lexemes.buf[state.next_lexeme_index].source_location, "Expected end of file");
    } else {
        return root_node;
    }
}

struct ast_stmt_node* parse_statement(struct state* state) {
    struct ast_stmt_node* result = NULL;

    struct lexeme* open_paren = expect_paren_open(state);
    struct lexeme* head = expect_word(state);
    if (strcmp(head->word, "let") == 0) {
        // TODO: check if not keyword
        struct lexeme* variable_name = expect_word(state);
        struct ast_expr_node* expr = parse_expression(state);
        VEC_PUSH(ast, ((union ast_node) {
            .stmt = {
                .type = AST_STMT_LET,
                .source_location = open_paren->source_location,
                .let = {
                    .name = variable_name->word,
                    .expr = expr,
                },
            }
        }));
        result = &VEC_LAST(ast)->stmt;
    } else {
        compiler_error(&head->source_location, "Invalid statement keyword");
    }
    expect_paren_close(state);

    return result;
}

struct ast_expr_node* parse_expression(struct state* state) {
    struct lexeme* lexeme = advance_lexeme(state);
    switch (lexeme->type) {
        case LEX_INT_LITERAL:
            VEC_PUSH(ast, ((union ast_node) {
                .expr = {
                    .type = AST_EXPR_INT_LITERAL,
                    .source_location = lexeme->source_location,
                    .int_literal = lexeme->int_literal,
                }
            }));
            return &VEC_LAST(ast)->expr;

        case LEX_WORD:
            // TODO: check if not keyword
            VEC_PUSH(ast, ((union ast_node) {
                .expr = {
                    .type = AST_EXPR_VARIABLE,
                    .source_location = lexeme->source_location,
                    .variable = lexeme->word,
                }
            }));
            return &VEC_LAST(ast)->expr;

        case LEX_PAREN_OPEN: {
            struct lexeme* head = expect_word(state);
            if (strcmp(head->word, "+") == 0) {
                struct ast_expr_node* arg1 = parse_expression(state);
                struct ast_expr_node* arg2 = parse_expression(state);
                expect_paren_close(state);
                VEC_PUSH(ast, ((union ast_node) {
                    .expr = {
                        .type = AST_EXPR_BINARY_OP,
                        .source_location = head->source_location,
                        .binary_op = {
                            .type = AST_BINARY_OP_ADD,
                            .children = { arg1, arg2 },
                        },
                    }
                }));
                return &VEC_LAST(ast)->expr;
            } else if (strcmp(head->word, "-") == 0) {
                struct ast_expr_node* arg1 = parse_expression(state);
                struct ast_expr_node* arg2 = parse_expression(state);
                expect_paren_close(state);
                VEC_PUSH(ast, ((union ast_node) {
                    .expr = {
                        .type = AST_EXPR_BINARY_OP,
                        .source_location = head->source_location,
                        .binary_op = {
                            .type = AST_BINARY_OP_SUB,
                            .children = { arg1, arg2 },
                        },
                    }
                }));
                return &VEC_LAST(ast)->expr;
            } else {
                compiler_error(&head->source_location, "Invalid function name");
            }
        }

        default:
            compiler_error(&lexeme->source_location, "x");
    }
}

struct lexeme* advance_lexeme(struct state* state) {
    if (state->next_lexeme_index < lexemes.size) {
        return &lexemes.buf[state->next_lexeme_index++];
    } else {
        compiler_error(NULL, "Unexpected end of file");
    }
}

struct lexeme* expect_paren_open(struct state* state) {
    struct lexeme* lexeme = advance_lexeme(state);
    if (lexeme->type == LEX_PAREN_OPEN) {
        return lexeme;
    } else {
        compiler_error(&lexeme->source_location, "Expected opening parenthesis");
    }
}

void expect_paren_close(struct state* state) {
    struct lexeme* lexeme = advance_lexeme(state);
    if (lexeme->type != LEX_PAREN_CLOSE) {
        compiler_error(&lexeme->source_location, "Expected closing parenthesis");
    }
}

struct lexeme* expect_word(struct state* state) {
    struct lexeme* lexeme = advance_lexeme(state);
    if (lexeme->type == LEX_WORD) {
        return lexeme;
    } else {
        compiler_error(&lexeme->source_location, "Expected word");
    }
}
