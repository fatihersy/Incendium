#include "fmemory.h"

#include <stdlib.h>  // Required for: malloc(), free()
#include <string.h>

static memory_system_state* memory_system;

void memory_system_initialize() {

    memory_system = (memory_system_state*)malloc(sizeof(memory_system_state));
    memory_system->linear_memory_total_size = TOTAL_ALLOCATED_MEMORY;
    memory_system->linear_memory_allocated = 0;
    memory_system->linear_memory = 0;

    memory_system->linear_memory = malloc(memory_system->linear_memory_total_size);
}

void* allocate_memory_linear(u64 size, bool will_zero_memory) 
{
    if (memory_system->linear_memory_allocated + size > memory_system->linear_memory_total_size) {
        u64 remaining = memory_system->linear_memory_total_size - memory_system->linear_memory_allocated;
        TraceLog(LOG_ERROR, "Tried to allocate %lluB, only %lluB remaining.", size, remaining);
        exit(EXIT_FAILURE);
    }

    void* block = ((u8*)memory_system->linear_memory) + memory_system->linear_memory_allocated;
    memory_system->linear_memory_allocated += size;

    if (will_zero_memory) memset(block, 0, size);

    return block;
}

void* allocate_memory(u64 size, bool will_zero_memory) {
    void* block = malloc(size);

    if (block == NULL) {
        TraceLog(LOG_FATAL, "BLOCK WAS NULL");
        exit(EXIT_FAILURE);
    }

    if (will_zero_memory) memset(block, 0, size);

    return block;
}

void free_memory(void* block) {
    free(block);
}

void zero_memory(void* block, u64 size) {
    memset(block, 0, size);
}
