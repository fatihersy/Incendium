#include "fmemory.h"

#include <stdlib.h> // Required for: malloc(), free()
#include <string.h> // Required for: memset(), memcpy()

#define TOTAL_ALLOCATED_MEMORY 512 * 1024 * 1024

typedef struct memory_system_state {
    unsigned long long linear_memory_total_size;
    unsigned long long  linear_memory_allocated;
    void *linear_memory;
  } memory_system_state;

static memory_system_state* memory_system;

void memory_system_initialize(void) {
    memory_system = (memory_system_state*)malloc(sizeof(memory_system_state));
    memory_system->linear_memory_total_size = TOTAL_ALLOCATED_MEMORY;
    memory_system->linear_memory_allocated = 0u;
    memory_system->linear_memory = nullptr;

    memory_system->linear_memory = malloc(memory_system->linear_memory_total_size);
}

void* allocate_memory_linear(unsigned long long  size, bool will_zero_memory) {
    #ifndef _RELEASE
      if (memory_system->linear_memory_allocated + size > memory_system->linear_memory_total_size) {
        exit(EXIT_FAILURE);
      }
      if (size % sizeof(size_t) != 0) {
        exit(EXIT_FAILURE);
      }
    #endif
    void* block = ((unsigned char*)memory_system->linear_memory) + memory_system->linear_memory_allocated;
    memory_system->linear_memory_allocated += size;

    if (will_zero_memory) memset(block, 0, size);

    return block;
}
void* allocate_memory(unsigned long long size, bool will_zero_memory) {
    void* block = malloc(size);

    if (block == NULL) {
        // TODO: 
        exit(EXIT_FAILURE);
    }
    if (will_zero_memory) memset(block, 0, size);

    return block;
}
void free_memory(void* block) {
    free(block);
}
void zero_memory(void* block, unsigned long long  size) {
    memset(block, 0, size);
}
void* copy_memory(void* dest, const void* source, unsigned long long  size) {
    return memcpy(dest, source, size);
}
void* set_memory(void* dest, int value, unsigned long long  size) {
    return memset(dest, value, size);
}
