#include "allocator.h"
#include <stdint.h>


typedef struct Block
{
    size_t size;
    struct Block *next;
    uint8_t is_free;
} Block;


struct Allocator
{
    void *memory;
    size_t size;
    Block *free_list;
};

 Allocator *allocator_create(void *const memory, const size_t size)
{
    if (size < sizeof(Allocator) + sizeof(Block))
        return NULL;

    Allocator *alloc = (Allocator *)memory;
    alloc->memory = memory;
    alloc->size = size;


    Block *initial_block = (Block *)((char *)memory + sizeof(Allocator));
    initial_block->size = size - sizeof(Allocator) - sizeof(Block);
    initial_block->next = NULL;
    initial_block->is_free = 1;

    alloc->free_list = initial_block;
    return alloc;
}

 void allocator_destroy(Allocator *const allocator)
{

    (void)allocator;
}

 void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    if (!allocator || !size)
        return NULL;

    Block *best_block = NULL;
    Block *prev_best = NULL;
    Block *current = allocator->free_list;
    Block *prev = NULL;
    size_t best_size = SIZE_MAX;

    // Find best fitting block
    while (current)
    {
        if (current->is_free && current->size >= size)
        {
            if (current->size < best_size)
            {
                best_size = current->size;
                best_block = current;
                prev_best = prev;
            }
        }
        prev = current;
        current = current->next;
    }

    if (!best_block)
        return NULL;


    if (best_block->size >= size + sizeof(Block) + 16)
    {
        Block *new_block = (Block *)((char *)best_block + sizeof(Block) + size);
        new_block->size = best_block->size - size - sizeof(Block);
        new_block->next = best_block->next;
        new_block->is_free = 1;

        best_block->size = size;
        best_block->next = new_block;
    }

    best_block->is_free = 0;
    return (char *)best_block + sizeof(Block);
}

 void allocator_free(Allocator *const allocator, void *const memory)
{
    if (!allocator || !memory)
        return;

    Block *block = (Block *)((char *)memory - sizeof(Block));
    block->is_free = 1;


    if (block->next && block->next->is_free)
    {
        block->size += block->next->size + sizeof(Block);
        block->next = block->next->next;
    }
}