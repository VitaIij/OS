#pragma once

/*
	classes of size 2^x -> 16, 32, ..., syspagesize / 2;
*/

enum State {FREE, CLASS, MULTIPAGE}; // 0 free, 1 blocks of same class, 2 multipage block

struct Describer {
	int* free_blocks_start;
	State state;				
	int free_blocks_count;
	int size_class;			// size of blocks or total bites of multipage(only for first page of multipage)
	Describer* next;		// next describer of the same class
	Describer();
};

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
	const size_t WSIZE = 4;
	const size_t min_class = 16;
	size_t page_size;
	int* heap_start;
	Describer* describers;							// array of all describers
	int* page_pointers;								// array of all pages
	int page_count;
	int class_count;								// amount of classes
	Describer** describers_free;						// array of lists of free pages of the same class
	size_t align_size(size_t size);
	size_t find_page_by_block(int* block);
	void* get_free_block(size_t index);
	void* configure_page(size_t size, size_t index);
	void set_multipage_block(Describer* describer, size_t pages);
	void* find_block(size_t size);
	void start_config();
};
