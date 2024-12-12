#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHARED_MEM_SIZE 4096
#define SEM_NAME "/my_semaphore"
#define SHARE_MEM_NAME "/my_shared_mem"
typedef struct
{
    char data[1024];
    int is_end;
    int has_data;
} shared_data;

int is_number(const char *str)
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }
    if (*str == '-')
    {
        str++;
    }
    while (*str != '\0')
    {
        if (!isdigit(*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

int safe_divide(int a, int b, int *error)
{
    if (b == 0)
    {
        *error = 1;
        return 0;
    }
    *error = 0;
    return a / b;
}

void write_to_file(int fd, const char *message)
{
    write(fd, message, strlen(message));
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        write(STDERR_FILENO, "Error: filename not provided\n", 28);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHARE_MEM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        write(STDERR_FILENO, "Error: Could not open shared memory\n", 35);
        exit(EXIT_FAILURE);
    }


    shared_data *shared_mem = (shared_data *)mmap(NULL, SHARED_MEM_SIZE,
                                                  PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        write(STDERR_FILENO, "Error: Could not map shared memory\n", 34);
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED)
    {
        write(STDERR_FILENO, "Error: Could not open semaphore\n", 31);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (output_fd == -1)
    {
        write(STDERR_FILENO, "Error: Could not open output file\n", 33);
        exit(EXIT_FAILURE);
    }

    char result_str[1024];
    while (1)
    {
  
        sem_wait(sem);
        if (shared_mem->has_data)
        {
            if (shared_mem->is_end && strcmp(shared_mem->data, "QUIT\n") == 0)
            {
                sem_post(sem);
      
                close(output_fd);
                sem_close(sem);
                munmap(shared_mem, SHARED_MEM_SIZE);
                exit(EXIT_SUCCESS);
            }

            char buffer[1024];
            strcpy(buffer, shared_mem->data);
            shared_mem->has_data = 0;
            sem_post(sem);

            char *token;
            int numbers[1024];
            int count = 0;
            int invalid_input = 0;

            token = strtok(buffer, " \n");
            while (token != NULL)
            {
                if (is_number(token))
                {
                    long val = strtol(token, NULL, 10);
                    if (val > INT_MAX || val < INT_MIN)
                    {
                        invalid_input = 1;
                        break;
                    }
                    numbers[count++] = (int)val;
                }
                else
                {
                    invalid_input = 1;
                    break;
                }
                token = strtok(NULL, " \n");
            }

            if (invalid_input || count < 2)
            {
                sprintf(result_str, "Invalid input\n");
                write_to_file(output_fd, result_str);
                continue;
            }

            int result = numbers[0];
            int division_error = 0;
            int i;

            for (i = 1; i < count; i++)
            {
                result = safe_divide(result, numbers[i], &division_error);
                if (division_error)
                {
                    write_to_file(output_fd, "Division by zero detected. Exiting.\n");
                    sem_wait(sem);
                    shared_mem->is_end = 1;
                    strcpy(shared_mem->data, "DIVISION_BY_ZERO");
                    shared_mem->has_data = 1;
                    sem_post(sem);

                    close(output_fd);
                    sem_close(sem);
                    munmap(shared_mem, SHARED_MEM_SIZE);
                    exit(EXIT_FAILURE);
                }
            }

            sprintf(result_str, "Result: %d\n", result);
            write_to_file(output_fd, result_str);
        }
        else
        {
            sem_post(sem);
            usleep(10000); 
        }
    }

    close(output_fd);
    sem_close(sem);
    munmap(shared_mem, SHARED_MEM_SIZE);
    return 0;
}