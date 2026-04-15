#include "MMU.h"

// Constructor
mmu::mmu(int size, char default_initial_value, int page_size)
{
    if (size <= 0) size = 1024;
    if (page_size <= 0) page_size = 64;

    memory_size_ = size;
    page_size_ = page_size;
    total_blocks_ = memory_size_ / page_size_;

    memory_ = new unsigned char[memory_size_];
    handle_table_ = new int[total_blocks_];

    for (int i = 0; i < memory_size_; i++)
        memory_[i] = default_initial_value;

    for (int i = 0; i < total_blocks_; i++)
        handle_table_[i] = 0;  // 0 = free

    next_handle_ = 1;
}

// Destructor
mmu::~mmu()
{
    delete[] memory_;
    delete[] handle_table_;
}

// Allocate memory
int mmu::Mem_Alloc(int size)
{
    if (size <= 0) return -1;

    int needed_blocks = (size + page_size_ - 1) / page_size_;

    // find free blocks
    for (int i = 0; i <= total_blocks_ - needed_blocks; i++)
    {
        bool free = true;

        for (int j = 0; j < needed_blocks; j++)
        {
            if (handle_table_[i + j] != 0)
            {
                free = false;
                break;
            }
        }

        if (free)
        {
            int handle = next_handle_++;

            // mark blocks
            for (int j = 0; j < needed_blocks; j++)
                handle_table_[i + j] = handle;

            // mark memory
            int start = i * page_size_;
            int end = start + needed_blocks * page_size_;

            for (int k = start; k < end; k++)
                memory_[k] = 'A' + (handle % 26);

            return handle;
        }
    }

    return -1; // no space
}

// Free memory
int mmu::Mem_Free(int memory_handle)
{
    if (memory_handle <= 0) return -1;

    bool found = false;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == memory_handle)
        {
            found = true;
            handle_table_[i] = 0;

            int start = i * page_size_;
            int end = start + page_size_;

            for (int j = start; j < end; j++)
                memory_[j] = '#';
        }
    }

    return found ? 0 : -1;
}