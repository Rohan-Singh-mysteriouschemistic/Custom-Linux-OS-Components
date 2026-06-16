#define _GNU_SOURCE
#include "loader.h"
#include <signal.h>
#include <string.h>

#define PAGE_SIZE 4096
#define MAX_MAPPED_PAGES 10000


Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;


int page_fault_count = 0;
int page_alloc_count = 0;
size_t total_fragmentation = 0;

void* mapped_pages[MAX_MAPPED_PAGES];

//protection flags
int getProtectionFlag(Elf32_Word p_flags) {
  int prot = PROT_NONE;
  if (p_flags & PF_R) prot = prot | PROT_READ;
  if (p_flags & PF_W) prot = prot | PROT_WRITE;
  if (p_flags & PF_X) prot = prot | PROT_EXEC;
  return prot;
}

void SignalHandler(int sig, siginfo_t *si, void *unused) 
{
  page_fault_count++;
  void* fault_addr = si->si_addr;

  // Align the faulting address down to the start of its 4KB page
  void* page_start = (void*)((unsigned long)fault_addr & ~(PAGE_SIZE - 1));

  int segment_found = 0;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type != PT_LOAD) {
      continue;
    }

    Elf32_Addr seg_start = phdr[i].p_vaddr;
    Elf32_Addr seg_end = seg_start + phdr[i].p_memsz;

    //checking if fault address is in the segment
    if ((unsigned long)fault_addr >= seg_start && (unsigned long)fault_addr < seg_end) 
    {
      segment_found = 1;
      
      
      page_alloc_count++; 

      //mapping memory
      void* new_page = mmap(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE,MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

      if (new_page == MAP_FAILED) 
      {
        perror("mmap in handler failed");
        exit(1);
      }
      
      if (page_alloc_count <= MAX_MAPPED_PAGES) 
      {
        mapped_pages[page_alloc_count - 1] = new_page;
      } 
      else 
      {
        fprintf(stderr, "error : exceeded max mappable pages\n");
        exit(1);
      }

      //load data for page
      Elf32_Addr page_offset_in_seg = (unsigned long)page_start - seg_start;
      Elf32_Addr file_offset = phdr[i].p_offset + page_offset_in_seg;
      
      
      if (page_offset_in_seg < phdr[i].p_filesz) {
        lseek(fd, file_offset, SEEK_SET);

        // Calculate how much to read
        size_t bytes_to_read = PAGE_SIZE;
        if (page_offset_in_seg + PAGE_SIZE > phdr[i].p_filesz) 
        {
          bytes_to_read = phdr[i].p_filesz - page_offset_in_seg;
        }

        ssize_t sizeRead = read(fd, page_start, bytes_to_read);
        if (sizeRead == -1) 
        {
          perror("error reading segment data in handler");
          exit(1);
        }
      }

      //permissions
      int final_prot = getProtectionFlag(phdr[i].p_flags);
      if (mprotect(page_start, PAGE_SIZE, final_prot) == -1) {
          perror("mprotect in handler failed");
          exit(1);
      }

      //internal fragmentation
      void* page_end = page_start + PAGE_SIZE;

      if ((unsigned long)page_end > seg_end) {
          size_t bytes_used = seg_end - (unsigned long)page_start;
          total_fragmentation += (PAGE_SIZE - bytes_used);
      }

      break;
    }
  }

  if (!segment_found) 
  {
    fprintf(stderr, "Fatal: Segmentation fault at unmanaged address %p\n", fault_addr);
    exit(1);
  }
}


void loader_cleanup() {
  for (int i = 0; i < page_alloc_count; i++) {
    if (mapped_pages[i] != NULL) {
      if (munmap(mapped_pages[i], PAGE_SIZE) == -1) 
      {
        perror("munmap failed during cleanup");
        
      }
    }
  }
  if (ehdr != NULL) {
    free(ehdr);
    ehdr = NULL;
  }
  if (phdr != NULL) {
    free(phdr);
    phdr = NULL;
  }
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}


void load_and_run_elf(char** exe) {
  // file discriptor open
  fd = open(exe[1], O_RDONLY);
  if (fd == -1) {
    perror("Failed to open Elf file");
    return;
  }
  // elf header read
  ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
  if (!ehdr) {
    perror("malloc failed at ehdr");
    return;
  }
  size_t ehsize = read(fd, ehdr, sizeof(Elf32_Ehdr));
  if (ehsize != sizeof(Elf32_Ehdr)) 
  {
    perror("Error reading elf header");
    return;
  }

  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 || ehdr->e_type != ET_EXEC) 
  {
    perror("Not a valid ELF executable");
    return;
  }
  // program header read
  size_t phsize = ehdr->e_phentsize * ehdr->e_phnum;
  phdr = (Elf32_Phdr*)malloc(phsize);
  if (!phdr) {
    perror("malloc failed at phdr");
    return;
  }
  lseek(fd, ehdr->e_phoff, SEEK_SET);
  size_t phsizeRead = read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum);
  if (phsizeRead != phsize) {
    perror("Error reading program header");
    return;
  }

  //link with SignalHandler
  struct sigaction sa;
  memset(&sa, 0, sizeof(sigaction));
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = SignalHandler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1) 
  {
    perror("sigaction");
    return;
  }


  //locating entry point
  Elf32_Addr entry = ehdr->e_entry;
  int (*_start)() = (int (*)())entry;

  //start running the program
  int result = _start();
  printf("User _start return value = %d\n", result);

  
  printf("\n--- SimpleSmartLoader Stats ---\n");
  printf("Total Page Faults: %d\n", page_fault_count);
  printf("Total Page Allocations: %d\n", page_alloc_count);
  printf("Total Internal Fragmentation: %.2f KB\n", (double)total_fragmentation / 1024.0);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }

  load_and_run_elf(argv);
 
  loader_cleanup();
  return 0;
}