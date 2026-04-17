#ifndef MMU_H
#define MMU_H

class mmu
{
private:
    unsigned char* memory_;
    int memory_size_;
    int page_size_;
    int total_blocks_;

    int next_handle_;

    int* handle_table_;   // stores which handle owns each block (0 = free)

public:
    mmu(int size, char default_initial_value, int page_size);
    ~mmu();

    int Mem_Alloc(int size);
    int Mem_Free(int memory_handle);

    unsigned char* get_memory() const { return memory_; }

    int Mem_Left();      // return the amount of core memory left in the OS
    int Mem_Largest();   // return the size of the largest available memory segment
    int Mem_Smallest();  // return the size of the smallest available memory segment
    int Mem_Coalesce();  // combine two or more contiguous blocks of free space, and place '.' (dots) in the coalesced memory
};

#endif