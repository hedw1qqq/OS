#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct Allocator Allocator;

typedef Allocator* (*allocator_create_fn)(void *const memory, const size_t size);
typedef void (*allocator_destroy_fn)(Allocator *const allocator);
typedef void* (*allocator_alloc_fn)(Allocator *const allocator, const size_t size);
typedef void (*allocator_free_fn)(Allocator *const allocator, void *const memory);


   


#endif 