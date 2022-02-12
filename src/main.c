#include "vxc.h"

int main(int argc, const char** argv) {
    FILE* file;
    if (argc == 1) {
        file = stdin;
    } else if (argc == 2) {
        file = fopen(argv[1], "r");
        if (!file) {
            perror(argv[1]);
            exit(3);
        }
    } else {
        fprintf(stderr, "Too many arguments\n");
        exit(2);
    }

    lex_file(file);

    if (file != stdin) {
        if (fclose(file)) {
            perror(argv[1]);
            exit(3);
        }
    }

    struct ast_stmt_node* ast_root = parse_lexemes();
    generate_code(ast_root);
    write_elf();

    fprintf(stderr, "String bytes: %6lu\n",          strings.size);
    fprintf(stderr, "Lexemes:      %6lu %8lu B\n", lexemes.size, VEC_BYTES(lexemes));
    fprintf(stderr, "AST nodes:    %6lu %8lu B\n", ast.size,     VEC_BYTES(ast));
    fprintf(stderr, "Code bytes:   %6lu\n",          program.size);
}

_Noreturn void compiler_error(struct source_location* location, const char* message) {
    if (location == NULL) {
        fprintf(stderr, "error: %s\n", message);
    } else {
        fprintf(stderr, "error at %u:%u: %s\n", location->line, location->column, message);
    }
    exit(1);
}
