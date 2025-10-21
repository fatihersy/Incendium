
#ifndef FMEMORY_H
#define FMEMORY_H

void memory_system_initialize(void);

void* allocate_memory(unsigned long long size, bool will_zero_memory);
void* allocate_memory_linear(unsigned long long  size, bool will_zero_memory);

void free_memory(void* block);
void zero_memory(void* block, unsigned long long  size);
void* copy_memory(void* dest, const void* source, unsigned long long  size);
void* set_memory(void* dest, int value, unsigned long long  size);

#endif
