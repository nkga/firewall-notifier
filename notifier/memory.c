#include "memory.h"
#include <Windows.h>

void* memory_alloc(size_t bytes) {
	return bytes ? HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes) : NULL;
}

void memory_free(void* ptr) {
	if (ptr) {
		HeapFree(GetProcessHeap(), 0, ptr);
	}
}
