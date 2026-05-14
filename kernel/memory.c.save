#include "memory.h"

#define HEAP_SIZE (16 * 1024 * 1024)  // 16MB heap

static uint8_t heap[HEAP_SIZE];
static uint32_t heap_offset = 0;

void memory_init(void) {
	heap_offset = 0;
}

void *kmalloc(uint32_t size) {
	// Align to 8 bytes
	size = (size + 7) & ~7;
	if (heap_offset + size > HEAP_SIZE) return 0;
	void *ptr = &heap[heap_offset];
	heap_offset += size;
	return ptr;
}

void kfree(void *ptr) {
	// Bump allocator - freeing not supported yet
	(void)ptr;
}
