#ifndef MMU_H
#define MMU_H

#include <vector>
#include <map>

struct crypto_meta
{
	std::vector<unsigned char> key;   // AES-256 key
	std::vector<unsigned char> iv;    // AES-CTR IV
};

class mmu {
private:
	unsigned char* memory_;
	int* handle_table_;

	int memory_size_;
	int page_size_;
	int total_blocks_;

	int next_handle_;

	std::map<int, crypto_meta> crypto_table_;

	int find_handle_start(int handle);

public:
	mmu(int size, char default_initial_value, int page_size);
	~mmu();

	int Mem_Alloc(int size);
	int Mem_Free(int memory_handle);
	int Mem_Free_NoCoalesce(int memory_handle);

	int Mem_Left();
	int Mem_Largest();
	int Mem_Smallest();
	int Mem_Coalesce();

	int Mem_Write(int handle, int offset, int size, char* data);
	int Mem_Read(int handle, int offset, int size, char* out);

	int get_block_handle(int block_index) const;

	// dump helpers used by ULTIMA
	int get_total_blocks() const { return total_blocks_; }
	int get_page_size() const { return page_size_; }
	int get_memory_size() const { return memory_size_; }
	const unsigned char* get_memory() const { return memory_; }
	void mmu_Mem_Dump();
};

#endif
