# Smart Loader Implementation

**Team Members**: Rohan Singh (2024477), Agrim Verma (2024047) 
**Course**: Operating Systems  

---

## Overview
This is a smart loader program which supports the following things: 
- Loads and run ELF files in demand paging manner.
- Maps pages on demand using SIGSEGV signal handler.

---

## Features
1. **Demand Paging**
   - Dynamically maps memory pages only when the process tries to access them.

2. **Dynamic Memory Mapping**  
   - Uses mmap() and mprotect() to allocate and protect memory pages.

4. **Page Statistics**  
   - Tracks the number of page faults, page allocations, and total internal fragmentation  
   - Proper cleanup of history and allocated command arrays is ensured.
     
5. **Cleanup and Resource Management**
   - Unmaps all allocated pages and closes file descriptors.
   - Frees ELF headers and resets all global states safely after program execution.

---

## Implimentation
   - When the user runs the loader with an ELF executable:
        - The loader opens and validates the ELF file.
        - ELF and Program Headers are read and parsed.
        - A custom signal handler is registered for SIGSEGV.
        - When a page fault occurs, the handler checks which segment caused it and maps the required page.
        - Data is loaded from the ELF file into memory only for that page.
        - Internal fragmentation is computed if the segment ends mid-page.
        - Finally, execution starts from the ELF entry point.
        - After execution completes, statistics are printed showing total page faults, allocations, and fragmentation.
---

## Statistics Report Example

./bin/launch ./test/sum
User _start return value = 2048

--- SimpleSmartLoader Stats ---
Total Page Faults: 3
Total Page Allocations: 3
Total Internal Fragmentation: 7.89 KB

---

## Contribution
   -Agrim Verma:
      - Signal Handler, loader cleanup
   -Rohan Singh:
      - load_and_run_elf, protection flags
      
---

## Compilation & Execution
run: 
   - make clean
   - make
   - ./bin/launch ./test/fib
   - ./bin/launch ./test/sum

github repo link: https://github.com/agrim-00/OS_SmartLoader


