#include "stdafx.h"
#include "MemAllocator.h"
#include <Windows.h>
#include <iostream>

constexpr static size_t WSIZE = 4;
constexpr static size_t DEFAUL_CHUNK_SIZE = 1024;
constexpr static size_t CHUNK_OVERHEAD = 12;		// chunk start + chank end + pointer to next chunk
constexpr static size_t BLOCK_OVERHEAD = 8;			// block size and availability
constexpr static size_t DEFAUL_SIZE_WITHOUT_OVERHEAD = DEFAUL_CHUNK_SIZE - CHUNK_OVERHEAD;

MemAllocator::MemAllocator() {
	free_blocks_start = heap_start = NULL;
}

MemAllocator::~MemAllocator()
{
	if (heap_start == NULL) {
		return;
	}

	int* current_chunk = static_cast<int*>(heap_start);
	int* prev_chunk = current_chunk;

	do
	{
		current_chunk = reinterpret_cast<int*>(*(current_chunk + (*current_chunk & -2) / WSIZE - 1));
		HeapFree(GetProcessHeap(), 0, prev_chunk);
		prev_chunk = current_chunk;
	} while (current_chunk != NULL);
}

void* MemAllocator::mem_alloc(size_t size) {
	if (size <= 0) {
		return NULL;
	}

	size = align_size(size);
	void* blockptr = find_block(size);

	if (blockptr == NULL) {
		return NULL;
	}

	split_block(blockptr, size);
	int* casted_blockptr = static_cast<int*>(blockptr);
	*(casted_blockptr + *casted_blockptr / WSIZE + 1) = *casted_blockptr | 1;			// set block as used
	*casted_blockptr = *casted_blockptr | 1;											// set block as used
	return static_cast<int*>(blockptr) + 1;
}

void* MemAllocator::mem_realloc(void* addr, size_t size)
{
	if (size <= 0) {
		return NULL;
	}

	int* new_block = static_cast<int*>(mem_alloc(size));
	size_t old_size = *(static_cast<int*>(addr) - 1) & -2;

	if (size <= old_size) {
		for (size_t i = 0; i < size / WSIZE; i++)
		{
			*(new_block + i) = *(static_cast<int*>(addr) + i);
		}
	}
	else {
		for (size_t i = 0; i < old_size / WSIZE; i++)
		{
			*(new_block + i) = *(static_cast<int*>(addr) + i);
		}
	}

	mem_free(addr);
	return new_block;
}

void MemAllocator::mem_free(void* addr)
{
	int* blockptr = static_cast<int*>(addr);
	blockptr -= 1;

	*blockptr = *blockptr & -2;												// set block as free
	*(blockptr + *blockptr / WSIZE + 1) = *blockptr;						// set block as free
	blockptr = static_cast<int*>(coalesce_blocks(blockptr));
	int* temp = static_cast<int*>(free_blocks_start);
	free_blocks_start = blockptr;
	*(blockptr + 1) = reinterpret_cast<int>(temp);
}

void* MemAllocator::create_chunk(size_t size)
{
	void* chunkptr;

	if (size > DEFAUL_SIZE_WITHOUT_OVERHEAD) {
		size += CHUNK_OVERHEAD + BLOCK_OVERHEAD;
		chunkptr = alloc_heap_mem(size);
	}
	else {
		size = DEFAUL_CHUNK_SIZE;
		chunkptr = alloc_heap_mem(size);
	}

	if (chunkptr == NULL) {
		return NULL;
	}

	int* castedptr = static_cast<int*>(chunkptr);
	*castedptr = size | 1;														// set start marker of chunk
	*(castedptr + size / WSIZE - 2) = 1;										// set end marker of chunk
	*(castedptr + size / WSIZE - 1) = reinterpret_cast<int>(heap_start);		// set pointer to next chunk
	*(castedptr + size / WSIZE - 3) = size - CHUNK_OVERHEAD - BLOCK_OVERHEAD;	// set length of first block(end)
	*(castedptr + 1) = size - CHUNK_OVERHEAD - BLOCK_OVERHEAD;					// set length of first block(start)

	*(castedptr + 2) = NULL;											// set pointer to next free block
	heap_start = chunkptr;												// set pointer to new chunk as head of chunk list
	return castedptr + 1;
}

size_t MemAllocator::align_size(size_t size) {
	return WSIZE * ((size + WSIZE - 1) / WSIZE);
}

void* MemAllocator::find_block(size_t size)
{
	int* blockptr;
	int* temp;

	if (free_blocks_start == NULL) {
		free_blocks_start = create_chunk(size);

		if (free_blocks_start == NULL) {
			return NULL;
		}
	}

	blockptr = temp = static_cast<int*>(free_blocks_start);
	
	if (*blockptr >= size) {
		free_blocks_start = reinterpret_cast<int*>(*(blockptr + 1));	// set head of free list equal to next block
		return blockptr;
	}
	while (true)
	{
		if (*blockptr >= size) {
			*(temp + 1) = *(blockptr + 1);
			break;
		}

		temp = blockptr;
		blockptr = reinterpret_cast<int*>(*(blockptr + 1));

		if (blockptr == NULL) {
			blockptr = static_cast<int*>(create_chunk(size));

			if (blockptr == NULL) {
				return NULL;
			}
			break;
		}
	}


	return blockptr;
}

void* MemAllocator::coalesce_blocks(void* blockptr)
{
	int* casted_blockptr = static_cast<int*>(blockptr);

	if ((*(casted_blockptr - 1) & 1) == 0) {								//coalesce with previous
		int* prev_blockptr = casted_blockptr - *(casted_blockptr - 1) / WSIZE - 2;
		*prev_blockptr = *casted_blockptr + *prev_blockptr + 8;
		*(prev_blockptr + *prev_blockptr / WSIZE + 1) = *prev_blockptr;
		casted_blockptr = prev_blockptr;

		void* prev_block_ancestor = find_block_ancestor(prev_blockptr);
		if (prev_block_ancestor != prev_blockptr) {
			*(static_cast<int*>(prev_block_ancestor) + 1) = *(prev_blockptr + 1);
		}
		else {
			free_blocks_start = reinterpret_cast<void*>(*(static_cast<int*>(free_blocks_start) + 1));
		}
	}
	if ((*(casted_blockptr + *casted_blockptr / WSIZE + 2) & 1) == 0) {
		int* next_blockptr = casted_blockptr + *casted_blockptr / WSIZE + 2;
		*casted_blockptr = *casted_blockptr + *next_blockptr + 8;
		*(next_blockptr + *next_blockptr / WSIZE + 1) = *casted_blockptr;

		void* next_block_ancestor = find_block_ancestor(next_blockptr);
		if (next_block_ancestor != next_blockptr) {
			*(static_cast<int*>(next_block_ancestor) + 1) = reinterpret_cast<int>(next_blockptr + 1);
		}
		else {
			free_blocks_start = reinterpret_cast<void*>(*(static_cast<int*>(free_blocks_start) + 1));
		}
	}

	return casted_blockptr;
}

void MemAllocator::split_block(void* blockptr, size_t size)
{
	int* casted_blockptr = static_cast<int*>(blockptr);
	int block_size = *casted_blockptr;
	if (block_size == size || block_size - size < BLOCK_OVERHEAD + WSIZE) {
		return;
	}

	size_t new_block_size = block_size - size - BLOCK_OVERHEAD;
	*(casted_blockptr + block_size / WSIZE + 1) = new_block_size;			// set size of new second block
	*(casted_blockptr + size / WSIZE + 2) = new_block_size;					// set size of new second block
	*(casted_blockptr) = size;												// set new size of first block
	*(casted_blockptr + size / WSIZE + 1) = size;							// set new size of first block

	int* temp = static_cast<int*>(free_blocks_start);
	free_blocks_start = casted_blockptr + size / WSIZE + 2;
	*(static_cast<int*>(free_blocks_start) + 1) = reinterpret_cast<int>(temp);
}

void* MemAllocator::find_block_ancestor(void* blockptr)
{
	if (blockptr == free_blocks_start) {
		return blockptr;
	}

	int* temp = static_cast<int*>(free_blocks_start);
	while (true)
	{
		if (reinterpret_cast<void*>(*(temp + 1)) == blockptr) {
			return temp;
		}
		temp = reinterpret_cast<int*>(*(temp + 1));

		if (temp == NULL) {
			return NULL;
		}
	}
}

int MemAllocator::get_block_size(void* blockptr)
{
	return *static_cast<int*>(blockptr) / WSIZE;
}

void* MemAllocator::alloc_heap_mem(size_t size)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void MemAllocator::mem_dump()
{
	if (heap_start == NULL) {
		return;
	}
	size_t chunk_index = 1;
	size_t block_index = 1;
	int* current_chunk = static_cast<int*>(heap_start);
	int* current_block = current_chunk + 1;

	do
	{
		block_index = 1;
		std::cout << "chunk " << chunk_index << " - " << current_chunk << std::endl;
		do
		{
			std::cout << current_block << " free : " << (*current_block & 1) << " size: " << (*current_block & -2) << std::endl;
			current_block = current_block + (*current_block & -2) / WSIZE + 2;
			block_index++;
		} while (*current_block != 1);
		current_chunk = reinterpret_cast<int*>(*(current_chunk + (*current_chunk & -2) / WSIZE - 1));
		chunk_index++;
		current_block = current_chunk + 1;
	} while (current_chunk != NULL);

	std::cout << "free list: ";
	current_block = static_cast<int*>(free_blocks_start);
	do
	{
		std::cout << *current_block << " ; ";
		current_block = reinterpret_cast<int*>(*(current_block + 1));
	} while (current_block != NULL);
	std::cout << "\n\n";
}
