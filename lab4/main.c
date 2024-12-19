#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

typedef struct Allocator Allocator;
typedef Allocator *(*AllocatorCreate)(void *, size_t);
typedef void (*AllocatorDestroy)(Allocator *);
typedef void *(*AllocatorAlloc)(Allocator *, size_t);
typedef void (*AllocatorFree)(Allocator *, void *);

void write_to_stdout(const char *message)
{
    write(STDOUT_FILENO, message, strlen(message));
}

void write_to_stderr(const char *message)
{
    write(STDERR_FILENO, message, strlen(message));
}

int main(int argc, char *argv[])
{


    if (argc < 2)
    {
        write_to_stderr("Error: Path to the library is required.\n");
        return 1;
    }

    write_to_stdout("Loading the library...\n");
    void *handle = dlopen(argv[1], RTLD_NOW);
    if (!handle)
    {
        write_to_stderr("Error: Failed to load the library.\n");
        return 1;
    }

    AllocatorCreate allocator_create = (AllocatorCreate)dlsym(handle, "allocator_create");
    AllocatorDestroy allocator_destroy = (AllocatorDestroy)dlsym(handle, "allocator_destroy");
    AllocatorAlloc allocator_alloc = (AllocatorAlloc)dlsym(handle, "allocator_alloc");
    AllocatorFree allocator_free = (AllocatorFree)dlsym(handle, "allocator_free");

    if (!allocator_create || !allocator_destroy || !allocator_alloc || !allocator_free)
    {
        write_to_stderr("Error: One or more required functions not found.\n");
        dlclose(handle);
        return 1;
    }

    size_t pool_size = 1024 * 1024;
    write_to_stdout("Allocating memory pool with mmap...\n");
    void *memory_pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED)
    {
        write_to_stderr("Error: Failed to allocate memory pool with mmap.\n");
        dlclose(handle);
        return 1;
    }
    printf("Memory pool allocated at: %p\n", memory_pool);

    write_to_stdout("Creating allocator...\n");
    Allocator *allocator = allocator_create(memory_pool, pool_size);
    if (!allocator)
    {
        write_to_stderr("Error: Failed to create allocator.\n");
        munmap(memory_pool, pool_size);
        dlclose(handle);
        return 1;
    }
    printf("Allocator created.\n");

    size_t allocation_size = 256;
    printf("Allocating memory block of size %zu...\n", allocation_size);
    void *block = allocator_alloc(allocator, allocation_size);
    if (!block)
    {
        write_to_stderr("Error: Failed to allocate memory block.\n");
        allocator_destroy(allocator);
        munmap(memory_pool, pool_size);
        dlclose(handle);
        return 1;
    }
    printf("Memory block allocated at: %p\n", block);

    printf("Freeing allocated memory block at %p...\n", block);
    allocator_free(allocator, block);
    printf("Memory block freed.\n");

    write_to_stdout("Destroying allocator...\n");
    allocator_destroy(allocator);
    printf("Allocator destroyed.\n");

    write_to_stdout("Unmapping memory pool...\n");
    munmap(memory_pool, pool_size);
    printf("Memory pool unmapped.\n");

    write_to_stdout("Closing the library...\n");
    dlclose(handle);
    printf("Library closed.\n");

    write_to_stdout("Execution completed successfully.\n");
    return 0;
}