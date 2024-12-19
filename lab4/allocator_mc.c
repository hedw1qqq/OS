#include <stddef.h>
#include <stdint.h>

#define MAX_CLASSES 10

typedef struct BlockClass
{
    size_t block_size;
    void *free_list;
} BlockClass;

typedef struct Allocator
{
    BlockClass classes[MAX_CLASSES];
    void *memory_start;
    size_t total_size;
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size)
{
    Allocator *allocator = (Allocator *)memory;
    allocator->memory_start = (void *)((uintptr_t)memory + sizeof(Allocator));
    allocator->total_size = size - sizeof(Allocator);

    size_t class_size = 16;
    void *current = allocator->memory_start;

    for (int i = 0; i < MAX_CLASSES; i++)
    {
        allocator->classes[i].block_size = class_size;
        allocator->classes[i].free_list = current;
        size_t blocks = (allocator->total_size / MAX_CLASSES) / class_size;

        for (size_t j = 0; j < blocks; j++)
        {
            void **next = (void **)current;
            current = (void *)((uintptr_t)current + class_size);
            *next = current;
        }
        class_size *= 2;
    }

    return allocator;
}

void allocator_destroy(Allocator *const allocator)
{

}

void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    for (int i = 0; i < MAX_CLASSES; i++)
    {
        if (allocator->classes[i].block_size >= size && allocator->classes[i].free_list)
        {
            void *block = allocator->classes[i].free_list;
            allocator->classes[i].free_list = *(void **)block;
            return block;
        }
    }
    return NULL;
}

void allocator_free(Allocator *const allocator, void *const memory)
{
    for (int i = 0; i < MAX_CLASSES; i++)
    {
        if ((uintptr_t)memory % allocator->classes[i].block_size == 0)
        {
            *(void **)memory = allocator->classes[i].free_list;
            allocator->classes[i].free_list = memory;
            return;
        }
    }
}
