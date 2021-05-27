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
        case 0: printf("invalid\n"); break; /*correct?*/
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
 
    sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff); /*file start adress + offset of shtable*/
    sectionNames= sections + curr_header->e_shstrndx;   /*shtable address + offset of sections names table*/
    sectionsNamesStrings = map_start + sectionNames->sh_offset; /*fsections names table + offset of shtable*/
    sh_num = curr_header->e_shnum;
    if (dFlag == 1){
        /*TODO*/
    }
    printf("[index]\tsection_name\tsection_address\tsection_offset\tsection_size\tsection_type\n");
    printf("[%2d]\t%21s\t%7x\t%7x\t%7x\t%10s\n" , 0 , "" , 0 , 0 , 0 ,stringOfType(0));

    for ( i = 1; i < sh_num; i++){
        printf("[%2d]\t%21s\t%7x\t%7x\t%7x\t%10s\n", i, sectionsNamesStrings+sections[i].sh_name,
                                                     sections[i].sh_addr, sections[i].sh_offset,
                                                     sections[i].sh_size ,stringOfType(sections[i].sh_type));    
    }
}

void printSymbols(){
    // int i, j sh_num, sym_offset, sym_num;
    // Elf32_Shdr *sections;
    // Elf32_Shdr *sectionNames;
    // char *sectionsNamesStrings;
    // Elf32_Symbol *symbols;
    // Elf32_Shdr *symbolsNames;
    
    //     if (currentfd == -1){
    //     fprintf(stderr,"%s\n", "no file is mapped");
	//     exit(1);
	// }   
    // sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff); /*file start adress + offset of shtable*/
    // *sectionNames = sections + curr_header->e_shstrndx;   /*shtable address + offset of sections names table*/
    // sectionsNamesStrings = map_start + sectionNames->sh_offset; /*fsections names table + offset of shtable*/
    // sh_num = curr_header->e_shnum;
    //     if (dFlag == 1){
    //     /*TODO*/
    // }
    // for (i = 1; i < sh_num; i++) {
    //     if (sections[i].sh_type == SHT_SYMTAB) {
    //         symbols = (Elf32_Symbol *)(map_start + sections[i].sh_offset);
    //         symbolsNames = (Elf32_Shdr *)(sections + sections[i].sh_link);
    //         sym_num = (sections[i].sh_size)/sizeof(Elf32_Sym);
    //         printf("[index]\tvalue\tsection_index\tsection_name\tsymbol_name\n");
    //         for (j = 0, j < sym_num; j++) {
    //             printf("[%2d]\t%21s\t%7x\t%7x\t%7x\t%10s\n", j, symbols[j].st_value, symbols[j].st_shndx,  ,symbolsNames + symbols[j].st_name
    //             printf("[%d]\t\t%9d\t\t%d\t\t%s\t\t%s\n" , j , secsym[j].st_value , secsym[j].st_shndx , isValidIndex(secsym[j].st_shndx , section , names_pointer) , (char *)(map_start+secsym_names->sh_offset + secsym[j].st_name));

    //         }
    //     }
}

void relocationTables(){
    int i, j, sh_num, rel_offset, rel_num;
    Elf32_Shdr *sections = (Elf32_Shdr *)(map_start + curr_header->e_shoff); /*file start adress + offset of shtable*/
    sh_num = curr_header->e_shnum;
    Elf32_Rel *rels;
    int rels_num;
    printf(" Offset \t  Info  \n");
    for (i = 0; i < sh_num; i++) {
        if (sections[i].sh_type == 9){
            rel_offset = sections[i].sh_offset;
            rels_num = sections[i].sh_size / sizeof(Elf32_Rel);
            rels = (Elf32_Rel *)(map_start + rel_offset);
            for (j = 0; j < rels_num; j++) {
                printf("%8x\t%8x\n",rels[j].r_offset, rels[j].r_info);
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