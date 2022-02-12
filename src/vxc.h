#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/elf.h>

#define ARRAY_SIZEOF(array) (sizeof((array)) / sizeof((array)[0]))

#define VEC_PUSH(vec, item) { \
    assert((vec).size < ARRAY_SIZEOF((vec).buf)); \
    (vec).buf[(vec).size] = (item); \
    ++(vec).size; \
}
#define VEC_APPEND(vec, array) { \
    assert((vec).size + ARRAY_SIZEOF(array) <= ARRAY_SIZEOF((vec).buf)); \
    memcpy((vec).buf + (vec).size, (array), sizeof((array))); \
    (vec).size += ARRAY_SIZEOF(array); \
}
#define VEC_BYTES(vec) ((vec).size * sizeof((vec).buf[0]))

void compiler_error(const char*);

enum lexeme_type {
    LEX_PAREN_OPEN,
    LEX_PAREN_CLOSE,
    LEX_WORD,
    LEX_INT_LITERAL,
};

struct lexeme {
    enum lexeme_type type;
    union {
        const char* word;
        uint64_t int_literal;
    };
};

enum ast_expr_node_type {
    AST_EXPR_VARIABLE,
    AST_EXPR_INT_LITERAL,
    AST_EXPR_BINARY_OP,
};

enum ast_binary_op_type {
    AST_BINARY_OP_ADD,
    AST_BINARY_OP_SUB,
};

struct ast_expr_node {
    enum ast_expr_node_type type;
    union {
        const char* variable;
        uint64_t int_literal;
        struct {
            enum ast_binary_op_type type;
            struct ast_expr_node* children[2];
        } binary_op;
    };
};

enum ast_stmt_node_type {
    AST_STMT_LET,
    AST_STMT_SEQUENCE,
};

struct ast_stmt_node {
    enum ast_stmt_node_type type;
    union {
        struct {
            const char* name;
            struct ast_expr_node* expr;
        } let;
        struct {
            struct ast_stmt_node* children[2];
        } sequence;
    };
};

union ast_node {
    struct ast_expr_node expr;
    struct ast_stmt_node stmt;
};

#define AST_MAX_SIZE 4096
struct ast {
    size_t size;
    union ast_node buf[AST_MAX_SIZE];
};
extern struct ast ast;

struct ast_stmt_node* parse_lexemes();

#define PROGRAM_MAX_SIZE 65536
struct program {
    size_t size;
    uint8_t buf[PROGRAM_MAX_SIZE];
};
extern struct program program;

void generate_code();

void write_elf();
