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
};

#endif