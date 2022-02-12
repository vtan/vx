#pragma once

#include <assert.h>
#include <stdbool.h>
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
#define VEC_LAST(vec) (&(vec).buf[(vec).size - 1])

struct source_location {
    uint32_t line;
    uint32_t column;
};

_Noreturn void compiler_error(struct source_location*, const char*);

enum lexeme_type {
    LEX_PAREN_OPEN,
    LEX_PAREN_CLOSE,
    LEX_WORD,
    LEX_INT_LITERAL,
};

struct lexeme {
    enum lexeme_type type;
    struct source_location source_location;
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
    struct source_location source_location;
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
    struct source_location source_location;
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

#define STRINGS_MAX_SIZE 65536
struct strings {
    size_t size;
    char buf[STRINGS_MAX_SIZE];
};
extern struct strings strings;

#define LEXEMES_MAX_SZE 8192
struct lexemes {
    size_t size;
    struct lexeme buf[LEXEMES_MAX_SZE];
};
extern struct lexemes lexemes;

#define AST_MAX_SIZE 4096
struct ast {
    size_t size;
    union ast_node buf[AST_MAX_SIZE];
};
extern struct ast ast;

#define PROGRAM_MAX_SIZE 65536
struct program {
    size_t size;
    uint8_t buf[PROGRAM_MAX_SIZE];
};
extern struct program program;

void lex_file(FILE*);
struct ast_stmt_node* parse_lexemes();
void generate_code(struct ast_stmt_node* ast_root);
void write_elf();
