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

static void codegen_from_stmt(struct ast_stmt_node*);
static void codegen_from_expr(struct ast_expr_node*);
static uint8_t rbp_offset_of_variable(struct lexeme*);
static void reset_stack_to_var(size_t local_vars_before_loop);

void generate_code(struct ast_stmt_node* ast_root) {
    program_append(program_start);
    codegen_from_stmt(ast_root);
    program_append(program_exit);
}

static void codegen_from_stmt(struct ast_stmt_node* stmt) {
    switch (stmt->type) {
        case AST_STMT_LET:
            if (find_local_variable(stmt->let.name->word) != NULL) {
                compiler_error(
                    &stmt->source_location,
                    "Variable is already defined. Use set instead of let to assign a new value"
                );
            }
            codegen_from_expr(stmt->let.expr);
            program_append(((uint8_t[]) { 0x50 }));  // push rax
            VEC_PUSH(local_variables, stmt->let.name->word);
            break;

        case AST_STMT_SET: {
            codegen_from_expr(stmt->set.expr);
            static uint8_t buf[] = { 0x48, 0x89, 0x45, 0 }; // mov [rbp - __], rax
            int rbp_offset = rbp_offset_of_variable(stmt->set.name);
            buf[3] = (uint8_t) rbp_offset;
            program_append(buf);
            break;
        }

        case AST_STMT_SEQUENCE: {
            size_t local_vars_before_block = local_variables.size;

            struct ast_stmt_node* next = stmt;
            while (next != NULL) {
                assert(next->sequence.stmt != NULL);
                codegen_from_stmt(next->sequence.stmt);
                next = next->sequence.tail;
            }

            // TODO: remove this hack when we don't print the stack before exit anymore
            bool is_root = stmt->source_location.column == 1 && stmt->source_location.line == 1;
            if (!is_root) {
                reset_stack_to_var(local_vars_before_block);
            }
            break;
        }

        case AST_STMT_WHILE: {
            size_t ip_before_loop = program.size;
            codegen_from_expr(stmt->while_loop.condition);
            program_append(((uint8_t[]) { 0x48, 0x85, 0xc0 }));        // test rax, rax
            program_append(((uint8_t[]) { 0x0f, 0x84, 0, 0, 0, 0 }));  // jz $+offset
            size_t ip_jump_ahead = program.size;

            size_t local_vars_before_body = local_variables.size;
            codegen_from_stmt(stmt->while_loop.body);
            reset_stack_to_var(local_vars_before_body);

            program_append(((uint8_t[]) { 0xe9, 0, 0, 0, 0 }));        // jmp $-offset
            size_t ip_end = program.size;

            *((uint32_t*) (program.buf + ip_jump_ahead - 4)) =
                (uint32_t) (ip_end - ip_jump_ahead);
            *((uint32_t*) (program.buf + ip_end - 4)) =
                (uint32_t) (ip_before_loop - ip_end);
            break;
        }
    }
}

static void codegen_from_expr(struct ast_expr_node* expr) {
    switch (expr->type) {
        case AST_EXPR_VARIABLE: {
            static uint8_t buf[] = { 0x48, 0x8b, 0x45, 0 }; // mov rax, [rbp - __]
            int rbp_offset = rbp_offset_of_variable(expr->variable);
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

static uint8_t rbp_offset_of_variable(struct lexeme* lexeme) {
    const char** p = find_local_variable(lexeme->word);
    if (p == NULL) {
        compiler_error(&lexeme->source_location, "Variable not found");
    }
    int offset = p - local_variables.buf;
    if (offset > 15) {
        compiler_error(&lexeme->source_location, "Too many variables");
    }
    return 0xf8 - offset * 8;
}

static void reset_stack_to_var(size_t reset_size) {
    if (local_variables.size != reset_size) {
        size_t diff_bytes = 8 * (local_variables.size - reset_size);
        if (reset_size == 0) {
            program_append(((uint8_t[]) {
                0x48, 0x89, 0xec,  // mov rsp, rbp
            }));
        } else if (diff_bytes < 0x80) {
            program_append(((uint8_t[]) {
                0x48, 0x83, 0xc4, 0,  // add rsp, imm8
            }));
            program.buf[program.size - 1] = diff_bytes;
        } else {
            program_append(((uint8_t[]) {
                0x48, 0x81, 0xc4, 0, 0, 0, 0,  // add rsp, imm32
            }));
            *((uint32_t*) &program.buf[program.size - 4]) = diff_bytes;
        }
        local_variables.size = reset_size;
    }
}
