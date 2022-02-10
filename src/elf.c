#include "vxc.h"

void write_elf(size_t program_size) {
    Elf64_Ehdr header = {
        .e_ident = {
            0x7f, 'E', 'L', 'F',
            ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE,
            0,0,0,0,0,0,0,0,
        },
        .e_type = ET_REL,
        .e_machine = 0x3e,
        .e_version = 1,
        .e_entry = 0,
        .e_phoff = 0,
        .e_shoff = sizeof(Elf64_Ehdr),
        .e_flags = 0,
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = 0,
        .e_phnum = 0,
        .e_shentsize = sizeof(Elf64_Shdr),
        .e_shnum = 3,
        .e_shstrndx = 1,
    };

    const char section_name_null[] = "";
    const char section_name_shstrtab[] = ".shstrtab";
    const char section_name_text[] = ".text";

    Elf64_Shdr section_null = {0};
    Elf64_Shdr section_string_table = {
        .sh_name = sizeof(section_name_null),
        .sh_type = SHT_STRTAB,
        .sh_offset = sizeof(Elf64_Ehdr) + 3 * sizeof(Elf64_Shdr),
        .sh_size =
            sizeof(section_name_null)
            + sizeof(section_name_shstrtab)
            + sizeof(section_name_text),
        .sh_addralign = 1,
    };
    Elf64_Shdr section_text = {
        .sh_name = section_string_table.sh_name + sizeof(section_name_shstrtab),
        .sh_type = SHT_PROGBITS,
        .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
        .sh_offset = sizeof(Elf64_Ehdr) + 3 * sizeof(Elf64_Shdr) + section_string_table.sh_size,
        .sh_size = program_size,
        .sh_addralign = 16,
    };

    fwrite(&header, sizeof(Elf64_Ehdr), 1, stdout);
    fwrite(&section_null, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(&section_string_table, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(&section_text, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(section_name_null, sizeof(section_name_null), 1, stdout);
    fwrite(section_name_shstrtab, sizeof(section_name_shstrtab), 1, stdout);
    fwrite(section_name_text, sizeof(section_name_text), 1, stdout);
    fwrite(program, program_size, 1, stdout);
}
