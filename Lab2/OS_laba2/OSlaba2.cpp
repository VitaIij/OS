#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include "MemAllocator.h"

using namespace std;
int main()
{
	MemAllocator* al = new MemAllocator();
	void* p1 = al->mem_alloc(16);			
	void* p2 = al->mem_alloc(32);
	void* p3 = al->mem_alloc(64);
	void* p4 = al->mem_alloc(128);
	al->mem_dump();
	void* p5 = al->mem_alloc(2345);			
	al->mem_dump();
	al->mem_free(p2);
	al->mem_dump();
	void* p61 = al->mem_alloc(10);			
	void* p62 = al->mem_alloc(11);
	void* p63 = al->mem_alloc(12);
	al->mem_dump();
	void* p7 = al->mem_realloc(p3, 24);		

	void* p8 = al->mem_alloc(10000);
	al->mem_dump();

	delete al;
}
