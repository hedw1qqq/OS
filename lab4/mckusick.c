#include "allocator.h"
#include <stdint.h>

#define POWER_OF_2(x) (1 << (x))
#define MIN_ALLOC_SHIFT 4 
#define MAX_ALLOC_SHIFT 20 
#define NUM_BUCKETS (MAX_ALLOC_SHIFT - MIN_ALLOC_SHIFT + 1)

typedef struct Block
{
    struct Block *next;
    size_t size; 
} Block;

struct Allocator
{
    void *memory;
    size_t size;
    Block *free_lists[NUM_BUCKETS];
};


static size_t get_bucket_index(size_t size)
{
    size_t index = 0;
    size_t min_size = POWER_OF_2(MIN_ALLOC_SHIFT);

    while (size > min_size)
    {
        min_size <<= 1;
        index++;
    }

    return index < NUM_BUCKETS ? index : NUM_BUCKETS - 1;
}

Allocator *allocator_create(void *const memory, const size_t size)
{
    if (size < sizeof(Allocator) + sizeof(Block))
        return NULL;

    Allocator *alloc = (Allocator *)memory;
    alloc->memory = memory;
    alloc->size = size;

    for (int i = 0; i < NUM_BUCKETS; i++)
    {
        alloc->free_lists[i] = NULL;
    }

    size_t remaining = size - sizeof(Allocator);
    char *current = (char *)memory + sizeof(Allocator);

    while (remaining >= POWER_OF_2(MIN_ALLOC_SHIFT))
    {
   
        size_t block_size = POWER_OF_2(MIN_ALLOC_SHIFT);
        while (block_size * 2 <= remaining)
        {
            block_size *= 2;
        }

        Block *block = (Block *)current;
        block->size = block_size;

        size_t bucket = get_bucket_index(block_size);
        block->next = alloc->free_lists[bucket];
        alloc->free_lists[bucket] = block;

        current += block_size;
        remaining -= block_size;
    }

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


    size_t required_size = size + sizeof(Block);

    size_t block_size = POWER_OF_2(MIN_ALLOC_SHIFT);
    while (block_size < required_size)
    {
        block_size *= 2;
    }

    size_t bucket = get_bucket_index(block_size);

    while (bucket < NUM_BUCKETS)
    {
        if (allocator->free_lists[bucket])
 
            Block *block = allocator->free_lists[bucket];
            allocator->free_lists[bucket] = block->next;

            while (block->size >= required_size * 2 &&
                   get_bucket_index(block->size / 2) >= get_bucket_index(required_size))
            {
                size_t new_size = block->size / 2;

                Block *new_block = (Block *)((char *)block + new_size);
                new_block->size = new_size;

                size_t new_bucket = get_bucket_index(new_size);
                new_block->next = allocator->free_lists[new_bucket];
                allocator->free_lists[new_bucket] = new_block;

                block->size = new_size;
            }

       
            block->size = block_size;

    
            return (char *)block + sizeof(Block);
        }
        bucket++;
    }

    return NULL;
}

void allocator_free(Allocator *const allocator, void *const memory)
{
    if (!allocator || !memory)
        return;


    Block *block = (Block *)((char *)memory - sizeof(Block));

    size_t bucket = get_bucket_index(block->size);

    block->next = allocator->free_lists[bucket];
    allocator->free_lists[bucket] = block;

}