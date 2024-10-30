
#ifndef FMEMORY_H
#define FMEMORY_H

#include "defines.h"

void memory_system_initialize();

void* allocate_memory(u64 size, bool will_zero_memory);
void* allocate_memory_linear(u64 size, bool will_zero_memory);

void free_memory(void* block);
void zero_memory(void* block, u64 size);

#endif