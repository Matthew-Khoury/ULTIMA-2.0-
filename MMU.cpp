#include "MMU.h"

#include <openssl/rand.h>
#include <openssl/evp.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>

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

// Allocate memory with AES metadata
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

        if (!free) continue;

        int handle = next_handle_++;

        for (int j = 0; j < needed_blocks; j++)
            handle_table_[i + j] = handle;

        // ---------------- AES METADATA ----------------
        crypto_meta meta;

        meta.key.resize(32);
        meta.iv.resize(16);

        RAND_bytes(meta.key.data(), 32);
        RAND_bytes(meta.iv.data(), 16);

        crypto_table_[handle] = meta;

        return handle;
    }

    return -1;
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

            for (int j = start; j < end && j < memory_size_; j++)
                memory_[j] = '#';
        }
    }
    // if not found then return -1
    if (!found) return -1;

    //free memory is found -> Mem_Coalesce()
    Mem_Coalesce();
    return 0;
}

// write (use AES-CTR encryption)
int mmu::Mem_Write(int handle, int offset, int size, char* data)
{
    if (handle <= 0 || !data || size <= 0) return -1;

    auto it = crypto_table_.find(handle);
    if (it == crypto_table_.end()) return -1;

    crypto_meta& meta = it->second;

    vector<unsigned char> plain(data, data + size);
    vector<unsigned char> cipher(size);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    int len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,
        meta.key.data(), meta.iv.data());

    EVP_EncryptUpdate(ctx,
        cipher.data(), &len,
        plain.data(), plain.size());

    cipher.resize(len);

    EVP_CIPHER_CTX_free(ctx);

    int start = find_handle_start(handle);
    if (start < 0) return -1;

    for (int i = 0; i < (int)cipher.size() && (start + offset + i) < memory_size_; i++)
        memory_[start + offset + i] = cipher[i];

    std::cout << "[MMU] Encrypted write (AES-CTR)\n";

    return 0;
}

// read (use AES-CTR encryption)
int mmu::Mem_Read(int handle, int offset, int size, char* out)
{
    if (handle <= 0 || !out || size <= 0) return -1;

    auto it = crypto_table_.find(handle);
    if (it == crypto_table_.end()) return -1;

    crypto_meta& meta = it->second;

    int start = find_handle_start(handle);
    if (start < 0) return -1;

    vector<unsigned char> cipher;

    for (int i = 0; i < size && (start + offset + i) < memory_size_; i++)
        cipher.push_back(memory_[start + offset + i]);

    vector<unsigned char> plain(cipher.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    int len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,
        meta.key.data(), meta.iv.data());

    EVP_DecryptUpdate(ctx,
        plain.data(), &len,
        cipher.data(), cipher.size());

    plain.resize(len);

    EVP_CIPHER_CTX_free(ctx);

    memcpy(out, plain.data(), min(size, (int)plain.size()));
    std::cout << "[MMU] Decrypted read (AES-CTR)\n";

    return 0;
}

int mmu::Mem_Coalesce()
{
    int freed = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0)
        {
            int start = i * page_size_;

            for (int j = start; j < start + page_size_ && j < memory_size_; j++)
            {
                if (memory_[j] != '.')
                {
                    memory_[j] = '.';
                    freed++;
                }
            }
        }
    }

    return freed;
}

int mmu::Mem_Left()
{
    int free_blocks = 0;

    for (int i = 0; i < total_blocks_; i++)
        if (handle_table_[i] == 0)
            free_blocks++;

    return free_blocks * page_size_;
}

int mmu::Mem_Largest()
{
    int max_blocks = 0, current = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0)
        {
            current++;
            max_blocks = max(max_blocks, current);
        }
        else current = 0;
    }

    return max_blocks * page_size_;
}

int mmu::Mem_Smallest()
{
    int min_blocks = total_blocks_ + 1;
    int current = 0;

    for (int i = 0; i < total_blocks_; i++)
    {
        if (handle_table_[i] == 0)
            current++;
        else
        {
            if (current > 0)
                min_blocks = min(min_blocks, current);
            current = 0;
        }
    }

    if (current > 0)
        min_blocks = min(min_blocks, current);

    if (min_blocks == total_blocks_ + 1)
        return 0;

    return min_blocks * page_size_;
}

int mmu::get_block_handle(int block_index) const
{
    if (block_index < 0 || block_index >= total_blocks_)
        return -1;

    return handle_table_[block_index];
}

int mmu::find_handle_start(int handle)
{
    for (int i = 0; i < total_blocks_; i++)
        if (handle_table_[i] == handle)
            return i * page_size_;

    return -1;
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

            for (int j = start; j < end && j < memory_size_; j++)
                memory_[j] = '#';
        }
    }

    crypto_table_.erase(memory_handle);

    return found ? 0 : -1;
}

void mmu::mmu_Mem_Dump() {
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
