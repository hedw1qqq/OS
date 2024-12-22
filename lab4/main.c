#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include "allocator.h"

#define MEMORY_SIZE (10 * 1024 * 1024) 
#define NUM_ALLOCATIONS 1000
#define MAX_ALLOC_SIZE 8192

static void *system_memory = NULL;

Allocator *system_allocator_create(void *const memory, const size_t size)
{
    (void)memory;
    (void)size;
    return (Allocator *)system_memory;
}

void system_allocator_destroy(Allocator *const allocator)
{
    (void)allocator;
}

void *system_allocator_alloc(Allocator *const allocator, const size_t size)
{
    (void)allocator;
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void system_allocator_free(Allocator *const allocator, void *const memory)
{
    (void)allocator;
    munmap(memory, 0);
}

void print_block_info(int index, void *ptr, size_t size)
{
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "Block %d: Address = %p, Size = %zu bytes\n",
                       index, ptr, size);
    write(STDOUT_FILENO, buffer, len);
}

void print_results(double alloc_time, double free_time,
                   void **allocations, size_t *sizes, int alloc_count)
{
    char buffer[256];
    int len;

    len = snprintf(buffer, sizeof(buffer), "\n=== Memory Allocation Summary ===\n");
    write(STDOUT_FILENO, buffer, len);

    int success_count = 0;
    size_t total_size = 0;

    for (int i = 0; i < alloc_count; i++)
    {
        if (allocations[i])
        {
            print_block_info(i, allocations[i], sizes[i]);
            success_count++;
            total_size += sizes[i];
        }
    }


    len = snprintf(buffer, sizeof(buffer), "Successful allocations: %d/%d\n",
                   success_count, alloc_count);
    write(STDOUT_FILENO, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Total allocated memory: %zu bytes\n",
                   total_size);
    write(STDOUT_FILENO, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Allocation time: %.6f seconds\n", alloc_time);
    write(STDOUT_FILENO, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Deallocation time: %.6f seconds\n", free_time);
    write(STDOUT_FILENO, buffer, len);

}

int main(int argc, char **argv)
{
   
    system_memory = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);


    allocator_create_fn create_fn = system_allocator_create;
    allocator_destroy_fn destroy_fn = system_allocator_destroy;
    allocator_alloc_fn alloc_fn = system_allocator_alloc;
    allocator_free_fn free_fn = system_allocator_free;


    void *lib = NULL;
    if (argc > 1)
    {
        lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
        if (lib)
        {
            create_fn = (allocator_create_fn)dlsym(lib, "allocator_create");
            destroy_fn = (allocator_destroy_fn)dlsym(lib, "allocator_destroy");
            alloc_fn = (allocator_alloc_fn)dlsym(lib, "allocator_alloc");
            free_fn = (allocator_free_fn)dlsym(lib, "allocator_free");
        }
        else
        {
            const char *msg = "Failed to load library, using system allocator\n";
            write(STDERR_FILENO, msg, strlen(msg));
        }
    }

 
    Allocator *allocator = create_fn(system_memory, MEMORY_SIZE);
    if (!allocator)
    {
        const char *msg = "Failed to create allocator\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return 1;
    }

    void *allocations[NUM_ALLOCATIONS] = {0};
    size_t sizes[NUM_ALLOCATIONS] = {0};
    clock_t start, end;
    size_t total_allocated = 0;


    srand(time(NULL));
    start = clock();
    for (int i = 0; i < NUM_ALLOCATIONS; i++)
    {
        sizes[i] = rand() % MAX_ALLOC_SIZE + 1;
        allocations[i] = alloc_fn(allocator, sizes[i]);
        if (allocations[i])
            total_allocated += sizes[i];
    }
    end = clock();
    double alloc_time = ((double)(end - start)) / CLOCKS_PER_SEC;


    start = clock();
    for (int i = 0; i < NUM_ALLOCATIONS; i++)
    {
        if (allocations[i])
            free_fn(allocator, allocations[i]);
    }
    end = clock();
    double free_time = ((double)(end - start)) / CLOCKS_PER_SEC;

   
    print_results(alloc_time, free_time, allocations, sizes, NUM_ALLOCATIONS);


    destroy_fn(allocator);
    if (lib)
        dlclose(lib);

    munmap(system_memory, MEMORY_SIZE);

    return 0;
}