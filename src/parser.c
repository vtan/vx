#include "vxc.h"

struct ast ast = {0};

struct state {
    size_t next_lexeme_index;
};

static struct ast_stmt_node* parse_statement(struct state* state);
static struct ast_expr_node* parse_expression(struct state* state);

static struct lexeme* next_lexeme(struct state* state);
static struct lexeme* advance_lexeme(struct state* state);
static struct lexeme* expect_paren_open(struct state* state);
static void expect_paren_close(struct state* state);
static struct lexeme* expect_word(struct state* state);

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
            if (strcmp(next->word, "let") == 0 || strcmp(next->word, "set") == 0) {
                // TODO: check if not keyword
                struct lexeme* variable_name = expect_word(state);
                struct ast_expr_node* expr = parse_expression(state);
                VEC_PUSH(ast, ((union ast_node) {
                    .stmt = {
                        .type = strcmp(next->word, "let") == 0 ? AST_STMT_LET : AST_STMT_SET,
                        .source_location = open_paren->source_location,
                        .let = {
                            .name = variable_name,
                            .expr = expr,
                        },
                    }
                }));
                result = &VEC_LAST(ast)->stmt;

            } else if (strcmp(next->word, "while") == 0) {
                struct ast_expr_node* condition = parse_expression(state);
                struct ast_stmt_node* body = parse_statement(state);
                VEC_PUSH(ast, ((union ast_node) {
                    .stmt = {
                        .type = AST_STMT_WHILE,
                        .source_location = open_paren->source_location,
                        .while_loop = {
                            .condition = condition,
                            .body = body,
                        },
                    }
                }));
                result = &VEC_LAST(ast)->stmt;

            } else if (strcmp(next->word, "if") == 0) {
                VEC_PUSH(ast, ((union ast_node) {
                    .stmt = {
                        .type = AST_STMT_IF,
                        .source_location = open_paren->source_location,
                        .if_chain = {0},
                    }
                }));
                struct ast_stmt_node* if_head = &VEC_LAST(ast)->stmt;
                struct ast_stmt_node* last = if_head;

                while (next_lexeme(state)->type != LEX_PAREN_CLOSE) {
                    struct ast_expr_node* condition = parse_expression(state);
                    struct ast_stmt_node* body = parse_statement(state);

                    if (last->if_chain.condition != NULL) {
                        VEC_PUSH(ast, ((union ast_node) {
                            .stmt = {
                                .type = AST_STMT_IF,
                                .source_location = open_paren->source_location,
                                .if_chain = {0},
                            }
                        }));
                        struct ast_stmt_node* next = &VEC_LAST(ast)->stmt;
                        last->if_chain.next = next;
                        last = next;
                    }
                    last->if_chain.condition = condition;
                    last->if_chain.body = body;
                }
                if (if_head->if_chain.condition == NULL) {
                    compiler_error(&open_paren->source_location, "Empty if is not allowed");
                }
                result = if_head;

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
                if (seq_last->sequence.stmt == NULL) {
                    seq_last->sequence.stmt = next_statement;
                } else {
                    VEC_PUSH(ast, ((union ast_node) {
                        .stmt = {
                            .type = AST_STMT_SEQUENCE,
                            .source_location = seq_start.source_location,
                            .sequence = {
                                .stmt = next_statement,
                                .tail = NULL,
                            },
                        }
                    }));
                    seq_last->sequence.tail = &VEC_LAST(ast)->stmt;
                    seq_last = seq_last->sequence.tail;
                }
            }
            if (seq_last->sequence.stmt == NULL) {
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
                    .variable = lexeme,
                }
            }));
            return &VEC_LAST(ast)->expr;

        case LEX_PAREN_OPEN: {
            struct lexeme* name = expect_word(state);
            VEC_PUSH(ast, ((union ast_node) {
                .expr = {
                    .type = AST_EXPR_CALL,
                    .source_location = name->source_location,
                    .call = {
                        .name = name,
                        .args_head = NULL,
                    },
                }
            }));
            struct ast_expr_node* call_node = &VEC_LAST(ast)->expr;
            struct ast_expr_node** next_arg = &call_node->call.args_head;
            while (next_lexeme(state)->type != LEX_PAREN_CLOSE) {
                struct ast_expr_node* expr = parse_expression(state);
                VEC_PUSH(ast, ((union ast_node) {
                    .expr = {
                        .type = AST_EXPR_CALL_ARG,
                        .source_location = expr->source_location,
                        .call_arg = {
                            .expr = expr,
                            .next = NULL,
                        },
                    }
                }));
                *next_arg = &VEC_LAST(ast)->expr;
                next_arg = &(*next_arg)->call_arg.next;
            }
            expect_paren_close(state);
            return call_node;
        }

        case LEX_PAREN_CLOSE:
            compiler_error(&lexeme->source_location, "Unexpected closing parenthesis in expression");
    }
    assert(UNREACHABLE);
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
