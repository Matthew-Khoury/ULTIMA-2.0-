#include "MMU.h"
#include <iostream>
#include <iomanip>

using namespace std;

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
    // if not found then return -1
    if (!found) return -1;

    //free memory is found -> Mem_Coalesce()
    Mem_Coalesce();
    return 0;
}

// return the amount of core memory left in the OS
int mmu::Mem_Left()
{
    int free_blocks = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0) // available (free) block
            free_blocks++;
    }

    return free_blocks * page_size_;  // free_blocks * page_size_ = total memory left available
}

int mmu::Mem_Free_NoCoalesce(int memory_handle)
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

    if (!found) return -1;

    return 0;
}

int mmu::get_block_handle(int block_index) const
{
    if (block_index < 0 || block_index >= total_blocks_)
        return -1;

    return handle_table_[block_index];
}

// return the size of the largest available memory segment (block)
int mmu::Mem_Largest()
{
    int max_blocks = 0;
    int current_blocks = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0) // available (free) block
        {
            current_blocks++;
            if (current_blocks > max_blocks)
                max_blocks = current_blocks;
        }
        else
        {
            current_blocks = 0;
        }
    }
    return max_blocks * page_size_;  // max_blocks * page_size = size of largest available memory block
}

// return the size of the smallest available memory segment
int mmu::Mem_Smallest()
{
    int min_blocks = total_blocks_ +1;
    int current_blocks = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0) // available (free) block
        {
            current_blocks++;
        }
        else
        {
            if (current_blocks > 0 && current_blocks < min_blocks)
                min_blocks = current_blocks;
            current_blocks = 0;
        }
    }

    if (current_blocks > 0 && current_blocks < min_blocks)
        min_blocks = current_blocks;

    if (min_blocks == total_blocks_ + 1)
        return 0;  // no free memory available

    return min_blocks * page_size_;  // min_blocks * page_size = size of smallest available memory block
}

// combine two or more contiguous blocks of free space, and place '.' (dots) in the coalesced memory
int mmu::Mem_Coalesce()
{
    int total_bytes = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0) // available (free) block
        {
            int start = i * page_size_;
            int end = start + page_size_;

            for (int j = start; j < end; j++)
            {
                if (memory_[j] != '.') // avoid recounting
                {
                    memory_[j] = '.';
                    total_bytes++;
                }
            }
        }
    }
    return total_bytes;
}

int mmu:: Mem_Read(int memory_handle, char *c){


    if (memory_handle <= 0) return -1; // validate the handle

    bool found = false;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == memory_handle) //ensure to read from the whole block
        {
            found = true;

            int start = i * page_size_;
            int end = start + page_size_;

            for (int j = start; j < end; j++)
            {
                *c = memory_[j];//copies from memory into buffer
                c++;
            }
        }
    }
    if (found) return 0;
    return -1; // handle not found
}
int mmu:: Mem_Write(int memory_handle, char *c){

    if (memory_handle <= 0) return -1; // validate the handle

    bool found = false;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == memory_handle) //ensure to write to the whole block
        {
            found = true;

            int start = i * page_size_;
            int end = start + page_size_;

            for (int j = start; j < end; j++)
            {
                memory_[j] = *c; //copies from buffer into memory
                c++;
            }
        }
    }
    if (found) return 0;
    return -1; // handle not found
}

int mmu :: Mem_Read(int memory_handle, int offset_from_beg, int text_size, char *text){
    if (memory_handle <= 0) return -1; // validate the handle
    if (offset_from_beg < 0 || text_size < 0) return -1;

    int first_block = -1;
    int block_count = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == memory_handle)
        {
            if (first_block < 0)
                first_block = i;
            block_count++;
        }
        else if (first_block >= 0)
        {
            break;
        }
    }

    if (first_block < 0) return -1; // handle not found

    int total_size = block_count * page_size_;
    if (offset_from_beg + text_size > total_size)
        return -1;

    int start = first_block * page_size_ + offset_from_beg;
    int end = start + text_size;

    for (int j = start; j < end; j++)
    {
        *text = memory_[j]; //copies from memory into buffer
        text++;
    }
    return 0;
}
int mmu :: Mem_Write(int memory_handle, int offset_from_beg, int text_size, char *text){
    if (memory_handle <= 0) return -1; // validate the handle
    if (offset_from_beg < 0 || text_size < 0) return -1;

    int first_block = -1;
    int block_count = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == memory_handle)
        {
            if (first_block < 0)
                first_block = i;
            block_count++;
        }
        else if (first_block >= 0)
        {
            break;
        }
    }

    if (first_block < 0) return -1; // handle not found

    int total_size = block_count * page_size_;
    if (offset_from_beg + text_size > total_size)
        return -1;

    int start = first_block * page_size_ + offset_from_beg;
    int end = start + text_size;

    for (int j = start; j < end; j++)
    {
        memory_[j] = *text; //copies from buffer into memory
        text++;
    }
    return 0;
}

// Dump the contents of the memory (typically we dump the entire memory)
// TODO: Test and debug: this might have issues when page_size != 16
void mmu::mmu_Mem_Dump()
{
    cout << "Addr    HEX BYTES                                 | ASCII" << endl;
    cout << "-------------------------------------------------------------" << endl;

    const int bytes_per_line = 16; // TODO: this is a possible problem

    for (int i = 0; i < memory_size_; i += bytes_per_line)
    {
        // Memory address
        cout << setw(6) << setfill('0') << hex << i << "  ";

        // Hex dump with page boundaries
        for (int j = 0; j < bytes_per_line; j++)
        {
            int current_index = i + j;

            if (current_index < memory_size_)
            {
                // Insert page boundary (skip at start)
                if (j != 0 && current_index % page_size_ == 0)
                {
                    cout << "| ";
                }

                cout << setw(2) << setfill('0')
                     << hex << (int)memory_[current_index] << " ";
            }
            else
            {
                cout << "   ";
            }
        }

        // Separator between Hex and ASCII
        cout << "| ";

        // ASCII dump with page boundaries
        for (int j = 0; j < bytes_per_line; j++)
        {
            int current_index = i + j;

            if (current_index < memory_size_)
            {
                if (j != 0 && current_index % page_size_ == 0)
                {
                    cout << "|";
                }

                unsigned char c = memory_[current_index];
                cout << (isprint(c) ? (char)c : '.');
            }
        }

        cout << endl;

        // Block and handle information
        if ((i % page_size_) == 0)
        {
            int block_index = i / page_size_;
            cout << "        [Block " << dec << block_index
                 << " | Handle: " << handle_table_[block_index] << "]"
                 << endl;
        }
    }
    cout << "===================================================" << endl;

    // Reset the formatting
    cout << dec << setfill(' ');
}