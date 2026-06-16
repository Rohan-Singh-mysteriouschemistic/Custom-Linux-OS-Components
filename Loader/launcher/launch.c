#include "../loader/loader.h"

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  // opened file through file pointer to pass
  FILE *fp = fopen(argv[1],"rb");
  // checked if file pointer is valid
  if(fp == NULL)
  {
    perror("Can't open file\n");
    exit(1);
  }
  fclose(fp);
  // running loader
  load_and_run_elf(argv);

  // cleanup
  loader_cleanup();
  return 0;
}
