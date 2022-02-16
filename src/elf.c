#include "vxc.h"

void write_elf() {
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
        .e_shnum = 4,
        .e_shstrndx = 1,
    };

    const size_t section_count = 4;
    const char section_name_null[] = "";
    const char section_name_shstrtab[] = ".shstrtab";
    const char section_name_text[] = ".text";
    const char section_name_bss[] = ".bss";

    Elf64_Shdr section_null = {0};

    const size_t offset_shstrtab = sizeof(Elf64_Ehdr) + section_count * sizeof(Elf64_Shdr);
    Elf64_Shdr section_string_table = {
        .sh_name = sizeof(section_name_null),
        .sh_type = SHT_STRTAB,
        .sh_offset = offset_shstrtab,
        .sh_size =
            sizeof(section_name_null)
            + sizeof(section_name_shstrtab)
            + sizeof(section_name_text)
            + sizeof(section_name_bss),
        .sh_addralign = 1,
    };

    const size_t offset_text = offset_shstrtab + section_string_table.sh_size;
    Elf64_Shdr section_text = {
        .sh_name = section_string_table.sh_name + sizeof(section_name_shstrtab),
        .sh_type = SHT_PROGBITS,
        .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
        .sh_offset = offset_text,
        .sh_size = VEC_BYTES(program),
        .sh_addralign = 16,
    };

    const size_t offset_bss = offset_text + section_text.sh_size;
    Elf64_Shdr section_bss = {
        .sh_name = section_string_table.sh_name + sizeof(section_name_shstrtab) + sizeof(section_name_text),
        .sh_type = SHT_NOBITS,
        .sh_flags = SHF_ALLOC | SHF_WRITE,
        .sh_offset = offset_bss,
        .sh_size = 0x4000,
        .sh_addralign = 4,
    };

    fwrite(&header, sizeof(Elf64_Ehdr), 1, stdout);
    fwrite(&section_null, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(&section_string_table, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(&section_text, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(&section_bss, sizeof(Elf64_Shdr), 1, stdout);
    fwrite(section_name_null, sizeof(section_name_null), 1, stdout);
    fwrite(section_name_shstrtab, sizeof(section_name_shstrtab), 1, stdout);
    fwrite(section_name_text, sizeof(section_name_text), 1, stdout);
    fwrite(section_name_bss, sizeof(section_name_bss), 1, stdout);
    fwrite(program.buf, VEC_BYTES(program), 1, stdout);
}
