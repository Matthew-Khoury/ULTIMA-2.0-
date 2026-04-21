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
	int Mem_Free_NoCoalesce(int memory_handle);
    int Mem_Read(int memory_handle, char * ch);
    int Mem_Write(int memory_handle, char * ch);
    int Mem_Read(int memory_handle, int offset_from_beg, int text_size, char *text);
    int Mem_Write(int memory_handle, int offset_from_beg, int text_size, char *text);

    unsigned char* get_memory() const { return memory_; }

    int Mem_Left();      // return the amount of core memory left in the OS
    int Mem_Largest();   // return the size of the largest available memory segment
    int Mem_Smallest();  // return the size of the smallest available memory segment
    int Mem_Coalesce();  // combine two or more contiguous blocks of free space, and place '.' (dots) in the coalesced memory

	int get_total_blocks() const { return total_blocks_; }
	int get_page_size() const { return page_size_; }
	int get_memory_size() const { return memory_size_; }
	int get_block_handle(int block_index) const;

    void mmu_Mem_Dump();
};

#endif