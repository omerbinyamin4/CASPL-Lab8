#define  NAME_LEN  128
#define  BUF_SZ    10000

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

char* stringOfType(int sh_type);

struct fun_desc{
    char *name;
    void (*fun)();
};
int dFlag = 0;
int currentfd = -1;
void *map_start;
struct stat currfd_stat;
Elf32_Ehdr *curr_header;

void ToggleDebugMode(){
    if (dFlag){
        dFlag = 0;
        printf("Debug flag now off\n");
    }
    else {
        dFlag = 1;
        printf("Debug flag now on\n");
    }
}

void examine(){
    if (currentfd != -1){
        munmap(map_start, currfd_stat.st_size);
        close(currentfd);
    }

    char fName[128];
    printf("Enter file name:\n");
    fgets(fName, 128, stdin);
    sscanf(fName, "%s", fName);

    currentfd = open(fName, O_RDONLY);
    if (currentfd < 0){
        perror("Failed to open file");
        exit(1);
    }
    if (fstat(currentfd, &currfd_stat) == -1){
        perror("Failed to stat");
        exit(1);
    }

    map_start = mmap(NULL, currfd_stat.st_size, PROT_READ, MAP_SHARED, currentfd, 0);
    if (map_start == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    curr_header = (Elf32_Ehdr *) map_start;

    if (curr_header->e_ident[1] != 'E' || curr_header->e_ident[2] != 'L' || curr_header->e_ident[3] != 'F'){
        fprintf(stderr, "Not an ELF File\n");
        munmap(map_start, currfd_stat.st_size);
        close(currentfd);
        currentfd = -1;
        exit(1);
    }
    printf("Magic: \t\t\t\t %x %x %x\n",curr_header->e_ident[1],curr_header->e_ident[2], curr_header->e_ident[3]);
    printf("Data: \t\t\t\t ");
    switch (curr_header->e_ident[5]){
        case 0: printf("invalid data encoding\n"); break;
        case 1: printf("2's complement, little endian\n"); break;
        case 2: printf("2's complement, big endian\n"); break;
    }
    printf("Entry point address:\t\t\t\t 0x%X\n",curr_header->e_entry);
    printf("Start of section headers:\t\t\t\t %d (bytes into file)\n",curr_header->e_shoff);
    printf("Number of section headers:\t\t\t\t %d\n",curr_header->e_shnum);
    printf("Size of section header entry:\t\t\t\t %d (bytes)\n",curr_header->e_shentsize);
    printf("Start of program headers:\t\t\t\t %d (bytes into file)\n",curr_header->e_phoff);
    printf("Number of program headers:\t\t\t\t %d\n",curr_header->e_phnum);
    printf("Size of program header entry:\t\t\t\t %d (bytes)\n",curr_header->e_phentsize);
}

void printSecNames(){
    int i, sh_num;
    Elf32_Shdr *sections;
    Elf32_Shdr *sectionNames;
    char *sectionsNamesStrings;
    if (currentfd == -1){
        fprintf(stderr,"%s\n", "no file is mapped");
	    exit(1);
	}
 
    sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff); /*file start adress + offset of section headers table*/
    sectionNames= sections + curr_header->e_shstrndx;   /*shtable address + offset of sections names table, 
                                                        this is an entry in the section headers table 
                                                        that contains the offset of the string table*/
    sectionsNamesStrings = map_start + sectionNames->sh_offset; /*file start adress + offset of strings table
                                                                    (which apears in sh_offset of the entry in 'section names'*/
    sh_num = curr_header->e_shnum;
    if (dFlag == 1){
        fprintf(stderr,"start address: [%p] + sections header table offset: [%d] = sections header table address: [%p]\n",map_start, curr_header->e_shoff, sections);
        fprintf(stderr,"sh_add: [%p] + names table offset: [%d] = section names entry: [%p]\n",sections,curr_header->e_shstrndx, sectionNames);
        fprintf(stderr,"start address: [%p] + strign table offset: [%d] = sections names string table address: [%p]\n",map_start, sectionNames->sh_offset, sectionsNamesStrings);
  
    }
    printf("[index]\tsection_name\tsection_address\tsection_offset\tsection_size\tsection_type\n");
    printf("[%2d]\t%21s\t%7x\t%7x\t%7x\t%10s\n" , 0 , "" , 0 , 0 , 0 ,stringOfType(0));

    for ( i = 1; i < sh_num; i++){
        printf("[%2d]\t%21s\t%7x\t%7x\t%7x\t%10s\n", i, sectionsNamesStrings+sections[i].sh_name,
                                                     sections[i].sh_addr, sections[i].sh_offset,
                                                     sections[i].sh_size ,stringOfType(sections[i].sh_type));    
    }
}

char* getSectionName(int section_number, Elf32_Shdr *sections, char* sectionsNamesStrings) {
    if (section_number < 1 || section_number > curr_header->e_shnum) return "ABS";
    else return sectionsNamesStrings+sections[section_number].sh_name;
}

void printSymbols(){
    int i, j, sh_num, sym_offset, sym_num;
    Elf32_Shdr *sections;
    Elf32_Shdr *sectionNames;
    char *sectionsNamesStrings;
    Elf32_Sym *symbols;
    Elf32_Shdr *sym_names_table;
    char *sym_names;
    
    if (currentfd == -1){
        fprintf(stderr,"%s\n", "no file is mapped");
	    exit(1);
	}

    sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff);
    sectionNames = sections + curr_header->e_shstrndx;   /*shtable address + offset of sections names table*/
    sectionsNamesStrings = map_start + sectionNames->sh_offset; /*fsections names table + offset of shtable*/
    sh_num = curr_header->e_shnum;
    if (dFlag == 1){
        fprintf(stderr,"start address: [%p] + sections header table offset: [%d] = sections header table address: [%p]\n",map_start, curr_header->e_shoff, sections);
        fprintf(stderr,"sh_add: [%p] + names table offset: [%d] = section names entry: [%p]\n",sections,curr_header->e_shstrndx, sectionNames);
        fprintf(stderr,"start address: [%p] + strign table offset: [%d] = sections names string table address: [%p]\n",map_start, sectionNames->sh_offset, sectionsNamesStrings);
  
    }
    for (i = 0; i < sh_num; i++) { //run through all sections
        if (sections[i].sh_type == SHT_SYMTAB || sections[i].sh_type == SHT_DYNSYM) {    // in case curr entry is symbol table
            symbols = (Elf32_Sym *)(map_start + sections[i].sh_offset); //define a new symbols table (add offset in curr entry)
            sym_names_table = (Elf32_Shdr *)(sections + sections[i].sh_link); // define its names table (linked to curr entry)
            sym_names = map_start + sym_names_table->sh_offset; // define the string of names linked to the names table
            sym_num = (sections[i].sh_size)/sizeof(Elf32_Sym); //calc number of entries in symbols table
            
            printf("[index]\t\tvalue\t\tsection_index\tsection_name\t\tsymbol_name\n");
            for (j = 0; j < sym_num; j++) {
                printf("[%2d]\t\t%d\t\t%d\t\t%s\t\t%s\n", j, symbols[j].st_value, symbols[j].st_shndx, 
                                                            getSectionName(symbols[j].st_shndx, sections, sectionsNamesStrings),
                                                            (char*)(sym_names + symbols[j].st_name));

            }
            printf("\n");
        }

    }
}

void relocationTables(){
    int i, j, sh_num, rel_offset, rels_num;
    Elf32_Shdr *sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff);
    sh_num = curr_header->e_shnum;
    Elf32_Rel *rels;
    Elf32_Sym *rel_linked_symTable;
    Elf32_Shdr *symTable_names;
    for (i = 0; i < sh_num; i++) {
        if (sections[i].sh_type == 9){
            rels = (Elf32_Rel *)(map_start + sections[i].sh_offset);
            rel_linked_symTable = (Elf32_Sym*)(map_start + sections[sections[i].sh_link].sh_offset);
            symTable_names = sections + sections[sections[i].sh_link].sh_link;
            rels_num = sections[i].sh_size / sizeof(Elf32_Rel);
            char * names = map_start + symTable_names->sh_offset;
            printf("Relocation section %s at offset %x contains %d entries:\n", names+sections[i].sh_name, sections[i].sh_offset, rels_num);
            printf(" offset \t  info  \tType\tSym.Value\tSym.Name\n");
            for (j = 0; j < rels_num; j++){
                printf("%08x\t%08x\t%4d\t %0x\t%s\n", rels[j].r_offset , rels[j].r_info , ELF32_R_TYPE(rels[j].r_info), 
                rel_linked_symTable[ELF32_R_SYM(rels[j].r_info)].st_value,
                names + rel_linked_symTable[ELF32_R_SYM(rels[j].r_info)].st_name);
            }
        }
    }
}

void quit(){
    if (currentfd != -1){
        if (dFlag)
            printf("Debug: unmapping...\n");
        munmap(map_start, currfd_stat.st_size);
        if (dFlag)
            printf("Debug: closing file...\n");
        close(currentfd);
        currentfd = -1;
    }
    if (dFlag)
            printf("Debug: all files are unmapped and closed\n");
    exit(0);
}


int main(int argc, char **argv) {
    struct fun_desc menu[7] = { {"Toggle Debug Mode", ToggleDebugMode},
                                {"Examine ELF File", examine},
                                {"Print Section Names", printSecNames},
                                {"Print Symbols", printSymbols},
                                {"Relocation Tables", relocationTables},
                                {"Quit", quit},
                                {NULL, NULL}
                                };

    char func_num[50];
    int op;
    int i;
    int bound;
    bound = sizeof(menu)/sizeof(struct fun_desc) - 2;

    while (1){
        op = -1;
        printf("choose action:\n");
        for (i=0; i < (sizeof(menu)/sizeof(struct fun_desc))-1; i++){
            printf("%i)-%s\n", i, (menu+i)->name);
        }
        printf("Option: ");
        fgets(func_num, 50, stdin);
        sscanf(func_num, "%d", &op);
            if (op < 0 || op > bound){
                printf("Not within bounds\n");
                exit(0);
            }
            else{
                (menu[func_num[0] - 48].fun)();
            }
        printf("\n");

    }
    
}

char* stringOfType(int sh_type) {
    switch (sh_type) {
        case 0 : return "SHT_NULL";
        case 1 : return "SHT_PROGBITS";
        case 2 : return "SHT_SYMTAB";
        case 3 : return "SHT_STRTAB";
        case 4 : return "SHT_RELA";
        case 5 : return "SHT_HASH";
        case 6 : return "SHT_DYNAMIC";
        case 7 : return "SHT_NOTE";
        case 8 : return "SHT_NOBITS";
        case 9 : return "SHT_REL";
        case 10 : return "SHT_SHLIB";
        case 11 : return "SHT_DYNSYM";
        case 0x70000000 : return "SHT_LOPROC";
        case 0x7fffffff : return "0x7fffffff";
        case 0x80000000 : return "SHT_LOUSER";
        case 0xffffffff : return "SHT_HIUSER";
    }
}