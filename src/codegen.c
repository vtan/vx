#include "vxc.h"

uint8_t program[PROGRAM_MAX_SIZE];
static uint8_t* program_next_byte = program;

static inline void program_push_sized(const uint8_t* bytes, size_t size) {
    if (program_next_byte - program + size > PROGRAM_MAX_SIZE) {
        compiler_error("Program size exceeded buffer limit");
    }
    memcpy(program_next_byte, bytes, size);
    program_next_byte += size;
}
#define program_push(bytes) (program_push_sized((bytes), sizeof(bytes)))

const char* local_variable_stack[256];
const char** local_variable_stack_next = local_variable_stack;

static inline void local_variable_push(const char* var) {
    if (local_variable_stack_next - local_variable_stack == sizeof(local_variable_stack)) {
        compiler_error("Local variable stack exceeded buffer limit");
    }
    *local_variable_stack_next = var;
    ++local_variable_stack_next;
}

static inline const char** local_variable_find(const char* var) {
    for (const char** p = local_variable_stack; p < local_variable_stack_next; ++p) {
        if (strcmp(var, *p) == 0) {
            return p;
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

static struct ast_node ast_nodes[] = {
    [0] = { .type = AST_SEQUENCE, .binary_children = { &ast_nodes[10], &ast_nodes[1] }, },
    [1] = { .type = AST_SEQUENCE, .binary_children = { &ast_nodes[20], &ast_nodes[2] }, },
    [2] = { .type = AST_SEQUENCE, .binary_children = { &ast_nodes[30], &ast_nodes[40] }, },

    [10] = { .type = AST_LET, .binary_children = { &ast_nodes[11], &ast_nodes[12] }, },
    [11] = { .type = AST_IDENTIFIER, .string = "a", },
    [12] = { .type = AST_ADD, .binary_children = { &ast_nodes[13], &ast_nodes[14] }, },
    [13] = { .type = AST_INT_LITERAL, .number = 0x11, },
    [14] = { .type = AST_INT_LITERAL, .number = 0x22, },

    [20] = { .type = AST_LET, .binary_children = { &ast_nodes[21], &ast_nodes[22] }, },
    [21] = { .type = AST_IDENTIFIER, .string = "b", },
    [22] = { .type = AST_SUB, .binary_children = { &ast_nodes[23], &ast_nodes[24] }, },
    [23] = { .type = AST_INT_LITERAL, .number = 0x100000011, },
    [24] = { .type = AST_INT_LITERAL, .number =  0xffffffff, },

    [30] = { .type = AST_LET, .binary_children = { &ast_nodes[31], &ast_nodes[32] }, },
    [31] = { .type = AST_IDENTIFIER, .string = "c", },
    [32] = { .type = AST_ADD, .binary_children = { &ast_nodes[33], &ast_nodes[34] }, },
    [33] = { .type = AST_INT_LITERAL, .number = 0x1000, },
    [34] = { .type = AST_SUB, .binary_children = { &ast_nodes[35], &ast_nodes[38] }, },
    [35] = { .type = AST_SUB, .binary_children = { &ast_nodes[36], &ast_nodes[37] }, },
    [36] = { .type = AST_INT_LITERAL, .number = 0x55, },
    [37] = { .type = AST_INT_LITERAL, .number = 0x11, },
    [38] = { .type = AST_INT_LITERAL, .number = 0x33, },

    [40] = { .type = AST_LET, .binary_children = { &ast_nodes[41], &ast_nodes[42] }, },
    [41] = { .type = AST_IDENTIFIER, .string = "d", },
    [42] = { .type = AST_SUB, .binary_children = { &ast_nodes[43], &ast_nodes[44] }, },
    [43] = { .type = AST_IDENTIFIER, .string = "c", },
    [44] = { .type = AST_ADD, .binary_children = { &ast_nodes[45], &ast_nodes[46] }, },
    [45] = { .type = AST_IDENTIFIER, .string = "a", },
    [46] = { .type = AST_IDENTIFIER, .string = "b", },
};

static void codegen_from_node(struct ast_node*);

size_t generate_code() {
    program_push(program_start);
    codegen_from_node(ast_nodes);
    program_push(program_exit);
    return program_next_byte - program;
}

static void codegen_from_node(struct ast_node* node) {
    switch (node->type) {
        case AST_SEQUENCE:
            codegen_from_node(node->binary_children[0]);
            codegen_from_node(node->binary_children[1]);
            break;

        case AST_LET:
            codegen_from_node(node->binary_children[1]);
            program_push(((uint8_t[]) { 0x50 }));  // push rax
            assert(node->binary_children[0]->type == AST_IDENTIFIER);
            local_variable_push(node->binary_children[0]->string);
            break;

        case AST_IDENTIFIER: {
            const char** p = local_variable_find(node->string);
            if (p == NULL) {
                compiler_error("Variable not found");
            }
            int offset = p - local_variable_stack;
            if (offset > 15) {
                compiler_error("Too many variables");
            }
            int rbp_offset = 0xf8 - offset * 8;
            static uint8_t buf[] = { 0x48, 0x8b, 0x45, 0 }; // mov rax, [rbp - __]
            buf[3] = (uint8_t) rbp_offset;
            program_push(buf);
            break;
        }

        case AST_INT_LITERAL:
            if (node->number <= 0xFFFFFFFF) {
                static uint8_t buf[] = { 0xb8, 0, 0, 0, 0 }; // mov eax, __
                *((uint32_t*) (buf + 1)) = (uint32_t) node->number;
                program_push(buf);
            } else {
                static uint8_t buf[] = { 0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0 }; // mov rax, __
                *((uint64_t*) (buf + 2)) = node->number;
                program_push(buf);
            }
            break;

        case AST_ADD:
            codegen_from_node(node->binary_children[1]);
            program_push(((uint8_t[]) { 0x50 }));  // push rax
            codegen_from_node(node->binary_children[0]);
            program_push(((uint8_t[]) {
                0x5b,              // pop rbx
                0x48, 0x01, 0xd8,  // add rax, rbx
            }));
            break;

        case AST_SUB:
            codegen_from_node(node->binary_children[1]);
            program_push(((uint8_t[]) { 0x50 }));  // push rax
            codegen_from_node(node->binary_children[0]);
            program_push(((uint8_t[]) {
                0x5b,              // pop rbx
                0x48, 0x29, 0xd8,  // sub rax, rbx
            }));
            break;
    }
}
