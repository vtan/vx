#include "vxc.h"

struct ast ast = {0};

struct state {
    size_t next_lexeme_index;
};

struct ast_stmt_node* parse_statement(struct state* state);
struct ast_expr_node* parse_expression(struct state* state);

struct lexeme* next_lexeme(struct state* state);
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
    struct lexeme* next = advance_lexeme(state);
    switch (next->type) {
        case LEX_WORD:
            if (strcmp(next->word, "let") == 0) {
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
                compiler_error(&next->source_location, "Invalid statement keyword");
            }
            break;

        case LEX_PAREN_OPEN:
            --state->next_lexeme_index;
            struct ast_stmt_node seq_start = {
                .type = AST_STMT_SEQUENCE,
                .source_location = open_paren->source_location,
            };
            struct ast_stmt_node* seq_last = &seq_start;

            while (next_lexeme(state)->type != LEX_PAREN_CLOSE) {
                struct ast_stmt_node* next_statement = parse_statement(state);
                if (seq_last->sequence.children[0] == NULL) {
                    seq_last->sequence.children[0] = next_statement;
                } else {
                    VEC_PUSH(ast, ((union ast_node) {
                        .stmt = {
                            .type = AST_STMT_SEQUENCE,
                            .source_location = seq_start.source_location,
                            .sequence = {
                                .children = { next_statement, NULL }
                            },
                        }
                    }));
                    seq_last->sequence.children[1] = &VEC_LAST(ast)->stmt;
                    seq_last = seq_last->sequence.children[1];
                }
            }
            if (seq_last->sequence.children[0] == NULL) {
                compiler_error(&seq_start.source_location, "Empty statement is not allowed");
            } else {
                VEC_PUSH(ast, ((union ast_node) { .stmt = seq_start }));
                result = &VEC_LAST(ast)->stmt;
            }
            break;

        default:
            compiler_error(&next->source_location, "Invalid statement body");
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

struct lexeme* next_lexeme(struct state* state) {
    if (state->next_lexeme_index < lexemes.size) {
        return &lexemes.buf[state->next_lexeme_index];
    } else {
        compiler_error(NULL, "Unexpected end of file");
    }
}

struct lexeme* advance_lexeme(struct state* state) {
    struct lexeme* lexeme = next_lexeme(state);
    ++state->next_lexeme_index;
    return lexeme;
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
