#include <elf.h>
#include <ftrace.h>
#include <stdio.h>

struct Func
{
    char *name;
    vaddr_t addr;
    struct Func *next;
};

static struct Func head;

static bool ftrace_en = true;

static unsigned int indents = 0;

void read_section(Elf64_Shdr *sh, void *ptr, FILE *file)
{
    fseek(file, sh->sh_offset, SEEK_SET);
    fread(ptr, sh->sh_size, 1, file);
}

void parse_elf(const char *elf_file)
{
    if (elf_file == NULL)
    {
        ftrace_en = false;
        return;
    }
    FILE *file = fopen(elf_file, "rb");
    Assert(file, "ELF file does not exist.");

    // read ELF header
    Elf64_Ehdr elf_header;
    fread(&elf_header, sizeof(elf_header), 1, file);
    Assert(memcmp(elf_header.e_ident, ELFMAG, SELFMAG) == 0, "Invalid ELF file.");

    // read section header
    Elf64_Shdr section_header[elf_header.e_shnum];
    fseek(file, elf_header.e_shoff, SEEK_SET);
    fread(&section_header, elf_header.e_shentsize, elf_header.e_shnum, file);

    // read symbol table
    Elf64_Shdr *symtab_shptr = NULL;
    Elf64_Sym *symtab_ptr = NULL;
    for (int i = 0; i < elf_header.e_shnum; i++)
    {
        if (section_header[i].sh_type == SHT_SYMTAB)
        {
            symtab_shptr = &section_header[i];
            symtab_ptr = malloc(symtab_shptr->sh_size);
            read_section(symtab_shptr, symtab_ptr, file);
        }
    }
    Assert(symtab_ptr != NULL, "Cannot find symtab.");

    // read string table
    Elf64_Shdr *strtab_shptr = &section_header[symtab_shptr->sh_link];
    char *strtab = malloc(strtab_shptr->sh_size);
    read_section(strtab_shptr, strtab, file);
    Assert(strtab_shptr != NULL, "Cannot find strtab.");

    // read functions
    int symbol_count = symtab_shptr->sh_size / sizeof(Elf64_Sym);
    struct Func *cur = &head;
    while (symbol_count--)
    {
        if (ELF64_ST_TYPE(symtab_ptr->st_info) == STT_FUNC)
        {
            cur->next = malloc(sizeof(Elf64_Sym));
            cur = cur->next;
            cur->name = strtab + symtab_ptr->st_name;
            cur->addr = symtab_ptr->st_value;
        }
        symtab_ptr++;
    }
    cur->next = NULL;

    fclose(file);
}

void check_call(vaddr_t addr)
{
    if (!ftrace_en)
        return;
    struct Func *cur = head.next;
    while (cur != NULL)
    {
        if (addr == cur->addr)
        {
            indents++;
            log_write("[ftrace] %*scall %s@" FMT_WORD_LH "\n", indents, " ", cur->name, addr);
            return;
        }
        cur = cur->next;
    }
}

void check_ret(uint32_t inst)
{
    if (!ftrace_en)
        return;
    if (inst == 0x8067)
    {
        log_write("[ftrace] %*sret\n", indents, " ");
    }
}