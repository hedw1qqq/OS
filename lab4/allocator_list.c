#include <stddef.h>
#include <stdint.h>

typedef struct FreeBlock {
    size_t size;
    struct FreeBlock* next;
} FreeBlock;

typedef struct Allocator {
    FreeBlock* free_list;
    void* memory_start;
    size_t total_size;
} Allocator;

Allocator* allocator_create(void *const memory, const size_t size) {
    Allocator* allocator = (Allocator*) memory;
    allocator->memory_start = (void*)((uintptr_t)memory + sizeof(Allocator));
    allocator->total_size = size - sizeof(Allocator);

    allocator->free_list = (FreeBlock*) allocator->memory_start;
    allocator->free_list->size = allocator->total_size - sizeof(FreeBlock);
    allocator->free_list->next = NULL;

    return allocator;
}

void allocator_destroy(Allocator *const allocator) {

}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    FreeBlock* current = allocator->free_list;
    FreeBlock** prev = &(allocator->free_list);

    while (current) {
        if (current->size >= size) {
            if (current->size >= size + sizeof(FreeBlock)) {
                FreeBlock* new_block = (FreeBlock*)((uintptr_t)current + sizeof(FreeBlock) + size);
                new_block->size = current->size - size - sizeof(FreeBlock);
                new_block->next = current->next;

                *prev = new_block;
            } else {
                *prev = current->next;
            }
            return (void*)((uintptr_t)current + sizeof(FreeBlock));
        }
        prev = &(current->next);
        current = current->next;
    }
    return NULL; 
}

void allocator_free(Allocator *const allocator, void *const memory) {
    FreeBlock* block_to_free = (FreeBlock*)((uintptr_t)memory - sizeof(FreeBlock));
    block_to_free->next = allocator->free_list;
    allocator->free_list = block_to_free;
}
