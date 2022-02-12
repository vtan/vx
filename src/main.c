#include "vxc.h"

int main() {
    struct ast_stmt_node* ast_root = parse_lexemes();
    generate_code(ast_root);
    write_elf();
}

_Noreturn void compiler_error(struct source_location* location, const char* message) {
    if (location == NULL) {
        fprintf(stderr, "error: %s\n", message);
    } else {
        fprintf(stderr, "error at %u:%u: %s\n", location->line, location->column, message);
    }
    exit(1);
}
