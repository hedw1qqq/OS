#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
typedef struct
{
    int *arr;
    int low;
    int cnt;
    int dir;
} thread_args;

void compare_and_swap(int *arr, int i, int j, int dir)
{
    if ((arr[i] > arr[j] && dir == 1) || (arr[i] < arr[j] && dir == 0))
    {
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void bitonic_merge(int *arr, int low, int cnt, int dir)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++)
            compare_and_swap(arr, i, i + k, dir);
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

void bitonic_sort_recursive(int *arr, int low, int cnt, int dir)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        bitonic_sort_recursive(arr, low, k, 1);
        bitonic_sort_recursive(arr, low + k, k, 0);
        bitonic_merge(arr, low, cnt, dir);
    }
}

void *parallel_bitonic_sort(void *args)
{
    thread_args *targs = (thread_args *)args;
    bitonic_sort_recursive(targs->arr, targs->low, targs->cnt, targs->dir);
    return NULL;
}

void parallel_bitonic_sort_main(int *arr, int size, int num_threads)
{

    if (num_threads <= 1)
    {
        bitonic_sort_recursive(arr, 0, size, 1);
        return;
    }

    if (num_threads > size / 2)
    {
        num_threads = size / 2;
    }

    pthread_t threads[num_threads];
    thread_args targs[num_threads];
    int chunk_size = size / num_threads;

    for (int i = 0; i < num_threads; i++)
    {
        targs[i].arr = arr;
        targs[i].low = i * chunk_size;
        targs[i].cnt = chunk_size;
        targs[i].dir = 1; 
        pthread_create(&threads[i], NULL, parallel_bitonic_sort, &targs[i]);
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Сливаем отсортированные части
    for (int j = chunk_size; j < size; j *= 2)
    {
        for (int i = 0; i < size; i += 2 * j)
        {
            if (i + j < size)
                bitonic_merge(arr, i, 2 * j, 1);
        }
    }
}

void write_to_stdout(const char *message)
{
    write(STDOUT_FILENO, message, strlen(message));
}
void write_to_stderr(const char *message)
{
    write(STDERR_FILENO, message, strlen(message));
}

int main()
{   srand(time(NULL));

    const int size = 1024;
    int arr[size];
    for (int i = 0; i < size;i++){
        arr[i] = rand() % 100;
    }
    
    if (size & (size - 1))
    {
        write_to_stderr("Массив должен быть степенью 2\n");
        return 1;

    }
    int num_threads = 16;


    write_to_stdout("Исходный массив: ");
    for (int i = 0; i < size; i++)
    {
        char result_buffer[50];
        snprintf(result_buffer, sizeof(result_buffer), "%d ", arr[i]);
        write(STDOUT_FILENO, result_buffer, strlen(result_buffer));
    }
    write_to_stdout("\n");

    parallel_bitonic_sort_main(arr, size, num_threads);

    write_to_stdout("Отсортированный массив: ");
    for (int i = 0; i < size; i++)
    {
        char result_buffer[50];
        snprintf(result_buffer, sizeof(result_buffer), "%d ", arr[i]);
        write(STDOUT_FILENO, result_buffer, strlen(result_buffer));
    }
    write_to_stdout("\n");

    return 0;
}