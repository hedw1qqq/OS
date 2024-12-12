#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>


#define SHARED_MEM_SIZE 4096
#define SEM_NAME "/my_semaphore"
#define SHARE_MEM_NAME "/my_shared_mem"

typedef struct shared_data
{
    char data[1024];
    int is_end;
    int has_data;
} shared_data;

void write_to_stdout(const char *message)
{
    write(STDOUT_FILENO, message, strlen(message));
}

int main()
{
    char filename[1024];
    pid_t child_pid;

    int shm_fd = shm_open(SHARE_MEM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        write_to_stdout("Error: Could not create shared memory\n");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHARED_MEM_SIZE) == -1)
    {
        write_to_stdout("Error: Could not set shared memory size\n");
        exit(EXIT_FAILURE);
    }

    shared_data *shared_mem = (shared_data *)mmap(NULL, SHARED_MEM_SIZE,
                                                  PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        write_to_stdout("Error: Could not map shared memory\n");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED)
    {
        write_to_stdout("Error: Could not create semaphore\n");
        exit(EXIT_FAILURE);
    }

    write_to_stdout("Enter filename: ");
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0)
    {
        write_to_stdout("Error: Could not read filename\n");
        exit(EXIT_FAILURE);
    }
    filename[bytes_read - 1] = '\0';

    shared_mem->is_end = 0;
    shared_mem->has_data = 0;

    child_pid = fork();

    if (child_pid == -1)
    {
        write_to_stdout("Error: Could not create child process\n");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0)
    {
    
        if (execl("./client", "client", filename, NULL) == -1)
        {
            write_to_stdout("Error: Could not start client program\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {

        char buffer[1024];
        write_to_stdout("Enter numbers separated by spaces (or 'q' to exit):\n");

        while (1)
        {
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0)
            {
                break;
            }
            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "q\n") == 0)
            {
                sem_wait(sem);
                shared_mem->is_end = 1;
                strcpy(shared_mem->data, "QUIT\n");
                shared_mem->has_data = 1;
                sem_post(sem);

                usleep(100000);

                wait(NULL);
                sem_close(sem);
                sem_unlink(SEM_NAME);
                munmap(shared_mem, SHARED_MEM_SIZE);
                shm_unlink(SHARE_MEM_NAME);
                exit(EXIT_SUCCESS);
            }


            sem_wait(sem);
            strcpy(shared_mem->data, buffer);
            shared_mem->has_data = 1;
            sem_post(sem);

            usleep(100000); 

            sem_wait(sem);
            if (shared_mem->is_end && strcmp(shared_mem->data, "DIVISION_BY_ZERO") == 0)
            {
                sem_post(sem);
                write_to_stdout("Child process detected division by zero. Terminating.\n");
                break;
            }
            sem_post(sem);
        }

        sem_close(sem);
        sem_unlink(SEM_NAME);
        munmap(shared_mem, SHARED_MEM_SIZE);
        shm_unlink(SHARE_MEM_NAME);
    }

    return 0;
}