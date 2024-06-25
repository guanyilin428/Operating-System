#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."
#define OS_SIZE_LOC_OFFSET 0x1fc

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen("image", "w");
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        fp = fopen(*files, "r");
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {
            if (options.extended == 1){
                printf("    segment%d\n", ph);
            }
            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;
	
    }
    write_os_size(nbytes,img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    fread(phdr, sizeof(Elf64_Phdr), 1, fp);
    if (options.extended == 1){
        printf("    offset: 0x%lx\n", phdr->p_offset);
        printf("    filesz: 0x%lx\n", phdr->p_filesz);
        printf("    memsz: 0x%lx\n", phdr->p_memsz);
        printf("    vaddr: 0x%lx\n", phdr->p_vaddr);
    }
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{   
    int sec_num = (phdr.p_filesz + 511)/512;
    int sec_sz  = sec_num * 512;
    char * buf = (char *)malloc(sec_sz * sizeof(char));
    for(int i=0; i < sec_sz; i++)
        buf[i] = 0;
    fseek(fp, phdr.p_offset, SEEK_SET);
    fread(buf, phdr.p_filesz, 1, fp);
    fseek(img, *nbytes, SEEK_SET);
    fwrite(buf, sec_sz, 1, img); 
    *nbytes += sec_sz;
}

static void write_os_size(int nbytes, FILE * img)
{
    short ker_num[1];
    ker_num[0] = nbytes / 512 - 1;
    fseek(img, OS_SIZE_LOC_OFFSET, SEEK_SET);
    fwrite(ker_num, sizeof(short), 1, img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
