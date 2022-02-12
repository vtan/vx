#include "vxc.h"

int main() {
    generate_code();
    write_elf();
}

void compiler_error(const char* message) {
    fprintf(stderr, "error: %s", message);
    exit(1);
}
