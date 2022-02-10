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

enum ast_node_type {
    AST_SEQUENCE,
    AST_LET,
    AST_IDENTIFIER,
    AST_INT_LITERAL,
    AST_ADD,
    AST_SUB,
};

struct ast_node {
    enum ast_node_type type;
    union {
        const char* string;
        uint64_t number;
        struct ast_node* binary_children[2];
    };
};

#define PROGRAM_MAX_SIZE 65536
extern uint8_t program[PROGRAM_MAX_SIZE];

size_t generate_code();

void write_elf(size_t program_size);
