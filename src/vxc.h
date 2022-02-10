#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/elf.h>

void compiler_error(const char*);

enum lexeme_type {
    LEX_PAREN_OPEN,
    LEX_PAREN_CLOSE,
    LEX_IDENTIFIER,
    LEX_INT_LITERAL,
    LEX_LET,
    LEX_PLUS,
    LEX_MINUS,
};

struct lexeme {
    enum lexeme_type type;
    union {
        const char* string;
        uint64_t number;
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

#define PROGRAM_MAX_SIZE 65536
extern uint8_t program[PROGRAM_MAX_SIZE];

size_t generate_code();

void write_elf(size_t program_size);
