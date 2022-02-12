#include "vxc.h"

struct program program = {0};

#define program_append(array) VEC_APPEND(program, array)

struct {
    size_t size;
    const char* buf[256];
} local_variables;

static inline const char** find_local_variable(const char* var) {
    for (size_t i = 0; i < local_variables.size; ++i) {
        if (strcmp(var, local_variables.buf[i]) == 0) {
            return &local_variables.buf[i];
        }
    }
    return NULL;
}

static const uint8_t program_start[] = {
    0x48, 0x89, 0xe5,              // mov rbp, rsp
};
static const uint8_t program_exit[] = {
    // write pushed variables to stdout
    0xb8, 0x01, 0x00, 0x00, 0x00,  // mov eax, 0x1
    0xbf, 0x01, 0x00, 0x00, 0x00,  // mov edi, 0x1
    0x48, 0x89, 0xe6,              // mov rsi, rsp
    0x48, 0x89, 0xea,              // mov rdx, rbp
    0x48, 0x29, 0xe2,              // sub rdx, rsp
    0x0f, 0x05,                    // syscall
    // exit
    0xb8, 0x3c, 0, 0, 0,  // mov eax, 0x3c
    0xbf, 0, 0, 0, 0,     // mov edi, 0
    0x0f, 0x05,           // syscall
};

static union ast_node ast_nodes[] = {
    [0] = { .stmt = {
        .type = AST_STMT_LET,
        .let = { .name = "a", .expr = &ast_nodes[12].expr }
    }},
    [12] = { .expr = {
        .type = AST_EXPR_BINARY_OP,
        .binary_op = { .type = AST_BINARY_OP_ADD, .children = { &ast_nodes[13].expr, &ast_nodes[14].expr }}
    }},
    [13] = { .expr = { .type = AST_EXPR_INT_LITERAL, .int_literal = 0x11 }},
    [14] = { .expr = { .type = AST_EXPR_INT_LITERAL, .int_literal = 0x22 }},
};

static void codegen_from_stmt(struct ast_stmt_node*);
static void codegen_from_expr(struct ast_expr_node*);

void generate_code() {
    program_append(program_start);
    codegen_from_stmt(&ast_nodes[0].stmt);
    program_append(program_exit);
}

static void codegen_from_stmt(struct ast_stmt_node* stmt) {
    switch (stmt->type) {
        case AST_STMT_LET:
            codegen_from_expr(stmt->let.expr);
            program_append(((uint8_t[]) { 0x50 }));  // push rax
            VEC_PUSH(local_variables, stmt->let.name);
            break;

        case AST_STMT_SEQUENCE:
            codegen_from_stmt(stmt->sequence.children[0]);
            codegen_from_stmt(stmt->sequence.children[1]);
            break;
    }
}

static void codegen_from_expr(struct ast_expr_node* expr) {
    switch (expr->type) {
        case AST_EXPR_VARIABLE: {
            const char** p = find_local_variable(expr->variable);
            if (p == NULL) {
                compiler_error("Variable not found");
            }
            int offset = p - local_variables.buf;
            if (offset > 15) {
                compiler_error("Too many variables");
            }
            int rbp_offset = 0xf8 - offset * 8;
            static uint8_t buf[] = { 0x48, 0x8b, 0x45, 0 }; // mov rax, [rbp - __]
            buf[3] = (uint8_t) rbp_offset;
            program_append(buf);
            break;
        }

        case AST_EXPR_INT_LITERAL:
            if (expr->int_literal <= 0xFFFFFFFF) {
                static uint8_t buf[] = { 0xb8, 0, 0, 0, 0 }; // mov eax, __
                *((uint32_t*) (buf + 1)) = (uint32_t) expr->int_literal;
                program_append(buf);
            } else {
                static uint8_t buf[] = { 0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0 }; // mov rax, __
                *((uint64_t*) (buf + 2)) = expr->int_literal;
                program_append(buf);
            }
            break;

        case AST_EXPR_BINARY_OP:
            codegen_from_expr(expr->binary_op.children[1]);
            program_append(((uint8_t[]) { 0x50 }));  // push rax
            codegen_from_expr(expr->binary_op.children[0]);
            program_append(((uint8_t[]) { 0x5b }));  // pop rbx

            switch (expr->binary_op.type) {
                case AST_BINARY_OP_ADD:
                    program_append(((uint8_t[]) {
                        0x48, 0x01, 0xd8,  // add rax, rbx
                    }));
                    break;

                case AST_BINARY_OP_SUB:
                    program_append(((uint8_t[]) {
                        0x48, 0x29, 0xd8,  // sub rax, rbx
                    }));
                    break;
            }
            break;
    }
}
