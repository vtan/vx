#include "vxc.h"

int main() {
    size_t program_size = generate_code();
    write_elf(program_size);
}

void compiler_error(const char* message) {
    fprintf(stderr, "error: %s", message);
    exit(1);
}
