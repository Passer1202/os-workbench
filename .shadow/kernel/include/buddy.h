#include <stdint.h>
#include <stddef.h>
#include <string.h>

void buddy_init(uintptr_t heap_start,uintptr_t heap_end);

void* buddy_alloc(size_t size);

void buddy_free(void* ptr);
//void print_mem_tree();