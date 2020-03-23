#pragma once

class MemAllocator
{
public:
	MemAllocator();
	~MemAllocator();
	void* mem_alloc(size_t size);
	void* mem_realloc(void* addr, size_t size);
	void mem_free(void* addr);
	void mem_dump();

private:
	void* heap_start;
	void* free_blocks_start;
	void* alloc_heap_mem(size_t size);
	size_t align_size(size_t size);
	void* create_chunk(size_t size);
	void* find_block(size_t size);
	void* coalesce_blocks(void* blockptr);
	void split_block(void* blockptr, size_t size);
	void* find_block_ancestor(void* blockptr);
	int get_block_size(void* blockptr);
};
