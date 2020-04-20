#include "stdafx.h"
#include "MemAllocator.h"
#include <Windows.h>
#include <iostream>
#include <sysinfoapi.h>

MemAllocator::MemAllocator() {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	page_size = info.dwPageSize / 2;
	page_count = 20;
	class_count = 0;
	size_t temp = page_size / 2;
	while (temp > 8)
	{
		class_count++;
		temp >>= 1;
	}
	heap_start = nullptr;
	describers = nullptr;
	page_pointers = nullptr;
	describers_free = nullptr;

	start_config();
}

MemAllocator::~MemAllocator()
{
	if (heap_start == nullptr) {
		return;
	}

	HeapFree(GetProcessHeap(), 0, heap_start);
	HeapFree(GetProcessHeap(), 0, page_pointers);
}

Describer::Describer()
{
	free_blocks_start = nullptr;
	state = FREE;
	free_blocks_count = 0;
	size_class = 0;
	next = nullptr;
}

void MemAllocator::start_config() {
	heap_start = (int*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, page_size * page_count);
	page_pointers = (int*)(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (page_count + class_count) * sizeof(int*) + page_count * sizeof(Describer)));		// memory for all arrays
	describers = reinterpret_cast<Describer*>(page_pointers + page_count);
	describers_free = reinterpret_cast<Describer**>(describers + page_count);
	for (size_t i = 0; i < page_count; i++)
	{
		describers[i].free_blocks_start = nullptr;
		describers[i].state = FREE;
		describers[i].free_blocks_count = 0;
		describers[i].size_class = 0;
		describers[i].next = nullptr;

		page_pointers[i] = reinterpret_cast<int>(heap_start + i * page_size / sizeof(int*));
	}

	for (size_t i = 0; i < class_count; i++)
	{
		describers_free[i] = &describers[i];
	}
}

void* MemAllocator::mem_alloc(size_t size) {
	if (size <= 0) {
		return nullptr;
	}

	size = align_size(size);
	return find_block(size);
}

void* MemAllocator::mem_realloc(void* addr, size_t size)
{
	if (size <= 0) {
		return NULL;
	}

	int* new_block = static_cast<int*>(mem_alloc(size));
	size_t index = find_page_by_block(static_cast<int*>(addr));
	size_t old_size = describers[index].size_class;

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
	int* block = static_cast<int*>(addr);
	size_t i = find_page_by_block(block), j;

	if (describers[i].state == MULTIPAGE)
	{
		for (size_t k = 0; k < describers[i].size_class / page_size; k++)
		{
			describers[k].state = FREE;
			describers[k].size_class = 0;
			return;
		}
	}

	for (j = 0; j < class_count; j++)										// find page with applicable class
	{
		if (min_class << j == describers[i].size_class)
		{
			break;
		}
	}

	describers[i].free_blocks_count++;
	*block = reinterpret_cast<int>(describers[i].free_blocks_start);
	describers[i].free_blocks_start = block;

	if (describers_free[j]->free_blocks_count != 0)
	{
		bool in_list = false;
		Describer* temp = describers_free[j];
		while (temp)
		{
			if (temp == describers + i)
			{
				in_list = true;
				break;
			}
			temp = temp->next;
		}

		if (!in_list)
		{
			describers[i].next = describers_free[j];
			describers_free[j] = &describers[i];
		}
	}
	else
	{
		describers_free[j] = &describers[i];
	}
}

size_t MemAllocator::find_page_by_block(int* block)
{
	for (size_t i = 0; i < page_count; i++)
	{
		if (reinterpret_cast<int*>(page_pointers[i]) <= block && block < reinterpret_cast<int*>(page_pointers[i + 1]))
		{
			return i;
		}
	}

	return -1;
}

size_t MemAllocator::align_size(size_t size)
{
	if (size > page_size / 2)
	{
		return page_size * ((size + page_size - 1) / page_size);
	}

	for (size_t i = 4; true; i++)
	{
		if (size <= pow(2, i))
		{
			return pow(2, i);
		}
	}
}

void* MemAllocator::find_block(size_t size)
{
	if (size > page_size / 2)
	{
		size_t pages = size / page_size;
		size_t describer_index = -1;
		for (size_t i = 0; i < page_count;)								// find describer of first page in multipage block(pages must be free)
		{
			for (size_t j = 0; j < pages && j + i < page_count; j++)
			{
				if (describers[i + j].state != FREE)
				{
					i = i + j + 1;
					break;
				}
				if (j == pages - 1)
				{
					describer_index = i;
				}
			}
			if (describer_index != -1)
			{
				set_multipage_block(describers + describer_index, pages);
				return reinterpret_cast<int*>(page_pointers[describer_index]);
			}
		}

		return nullptr;
	}

	for (size_t i = 0; i < class_count; i++)										// find page with applicable class
	{
		if (min_class << i == size)
		{
			if (describers_free[i]->free_blocks_count != 0)
			{
				return get_free_block(i);
			}
			else if (i + 1 != class_count && describers_free[i + 1]->free_blocks_count != 0)
			{
				return get_free_block(i + 1);
			}
			else {
				return configure_page(size, i);
			}
		}
	}

	return nullptr;
}

void MemAllocator::set_multipage_block(Describer* describer, size_t pages)
{
	for (size_t i = 0; i < pages; i++)
	{
		describer[i].state = MULTIPAGE;
		describer[i].size_class = pages * page_size;
	}
}

void* MemAllocator::get_free_block(size_t index)
{
	int* block = describers_free[index]->free_blocks_start;
	describers_free[index]->free_blocks_count--;
	describers_free[index]->free_blocks_start = reinterpret_cast<int*>(*block);
	return block;
}

void* MemAllocator::configure_page(size_t size, size_t index)
{
	for (size_t i = 0; i < page_count; i++)										// find first free page
	{
		if (describers[i].state == FREE)
		{
			describers[i].state = CLASS;
			describers[i].size_class = size;
			describers[i].free_blocks_count = page_size / size;
			describers[i].free_blocks_start = reinterpret_cast<int*>(page_pointers[i]);
			int* block = reinterpret_cast<int*>(page_pointers[i]);
			for (size_t j = 0; j < page_size / size - 1; j++)
			{
				*block = reinterpret_cast<int>(block + size / sizeof(int*));
				block += size / sizeof(int*);
			}
			*block = reinterpret_cast<int>(nullptr);
			describers[i].free_blocks_count--;
			describers[i].free_blocks_start = reinterpret_cast<int*>(*(describers[i].free_blocks_start));
			describers_free[index] = &describers[i];
			return reinterpret_cast<int*>(page_pointers[i]);
		}
	}
	return nullptr;
}

void MemAllocator::mem_dump()
{
	for (size_t i = 0; i < page_count; i++)
	{
		std::cout << "page: " << "\t" << i << "\tclass: " << "\t" << describers[i].size_class
			<< "\tstate:" << "\t" << describers[i].state << "\tfree blocks: " << "\t" << describers[i].free_blocks_count << "\n";
	}
	std::cout << "\n\n";
}
