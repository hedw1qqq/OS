#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
typedef struct {
    int *arr;
    int low;
    int high;
    int dir;
} thread_args;

void compare_and_swap(int *arr, int i, int j, int dir) {
    if ((arr[i] > arr[j] && dir == 1) || (arr[i] < arr[j] && dir == 0)) {
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void bitonic_merge(int *arr, int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            compare_and_swap(arr, i, i + k, dir);
        }
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

void bitonic_sort_recursive(int *arr, int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        bitonic_sort_recursive(arr, low, k, 1);
        bitonic_sort_recursive(arr, low + k, k, 0);
        bitonic_merge(arr, low, cnt, dir);
    }
}

void *parallel_bitonic_sort(void *args) {
    thread_args *targs = (thread_args *)args;
    bitonic_sort_recursive(targs->arr, targs->low, targs->high - targs->low + 1, targs->dir);
    return NULL;
}

void parallel_bitonic_sort_main(int *arr, int size, int num_threads) {
    if (num_threads <= 1) {
        bitonic_sort_recursive(arr, 0, size, 1);
        return;
    }

    pthread_t threads[num_threads];
    thread_args targs[num_threads];
    int chunk = size / num_threads;

    // сортировка
    for (int i = 0; i < num_threads; i++) {
        targs[i].arr = arr;
        targs[i].low = i * chunk;
        targs[i].high = fmin((i + 1) * chunk - 1, size - 1);

        targs[i].dir = 1;
        pthread_create(&threads[i], NULL, parallel_bitonic_sort, &targs[i]);
    }

    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // слияние
    for (int step = chunk; step < size; step *= 2) {
        int num_merges = size / (2 * step);
        int threads_to_use = (num_threads < num_merges) ? num_threads : num_merges;

        pthread_t merge_threads[threads_to_use];
        thread_args merge_args[threads_to_use];

        for (int i = 0; i < threads_to_use; i++) {
            int low = i * 2 * step;
            int mid = low + step - 1;
            int high = fmin(low + 2 * step - 1, size - 1);

            merge_args[i].arr = arr;
            merge_args[i].low = low;
            merge_args[i].high = high;
            merge_args[i].dir = 1;

            pthread_create(&merge_threads[i], NULL, parallel_bitonic_sort, &merge_args[i]);
        }

        for (int i = 0; i < threads_to_use; i++) {
            pthread_join(merge_threads[i], NULL);
        }
    }
}

void write_to_stdout(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void write_to_stderr(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write_to_stderr("Используйте: ./program <number_of_threads>\n");
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        write_to_stderr("Количество потоков >=0.\n");
        return 1;
    }

    srand(time(NULL));

    const int size = 16;
    int arr[size];
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 100;
    }

    if (size & (size - 1)) {
        write_to_stderr("Массив должен быть степенью 2.\n");
        return 1;
    }

    write_to_stdout("Исходный массив: ");
    for (int i = 0; i < size; i++) {
        char result_buffer[50];
        snprintf(result_buffer, sizeof(result_buffer), "%d ", arr[i]);
        write_to_stdout(result_buffer);
    }
    write_to_stdout("\n");

    parallel_bitonic_sort_main(arr, size, num_threads);


    write_to_stdout("Отсортированный массив: ");
    for (int i = 0; i < size; i++) {
        char result_buffer[50];
        snprintf(result_buffer, sizeof(result_buffer), "%d ", arr[i]);
        write_to_stdout(result_buffer);
    }
    write_to_stdout("\n");

    return 0;
}
