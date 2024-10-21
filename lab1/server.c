#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define READ_END 0
#define WRITE_END 1

void write_to_stdout(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

int main(int argc, char **argv) {
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];
    pid_t child_pid;
    char buffer[BUFFER_SIZE];
    char filename[BUFFER_SIZE];

    if (argc != 2) {
        write_to_stdout("Использование: ./program <имя_файла>\n");
        exit(EXIT_FAILURE);
    }

    strncpy(filename, argv[1], BUFFER_SIZE);
    filename[BUFFER_SIZE - 1] = '\0';

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        write_to_stdout("Не удалось создать трубу\n");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();

    if (child_pid == -1) {
        write_to_stdout("Не удалось создать дочерний процесс\n");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) { // Дочерний процесс
        close(pipe_parent_to_child[WRITE_END]);
        close(pipe_child_to_parent[READ_END]);

        dup2(pipe_parent_to_child[READ_END], STDIN_FILENO);
        dup2(pipe_child_to_parent[WRITE_END], STDOUT_FILENO);

        execl("./client", "client", NULL);

        write_to_stdout("Не удалось запустить клиентскую программу\n");
        exit(EXIT_FAILURE);
    } else { // Родительский процесс
        close(pipe_parent_to_child[READ_END]);
        close(pipe_child_to_parent[WRITE_END]);

        // Отправляем имя файла клиенту
        strcat(filename, "\n");
        write(pipe_parent_to_child[WRITE_END], filename, strlen(filename));

        write_to_stdout("Введите числа, разделенные пробелами (или 'q' для выхода):\n");

        while (1) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) {
                write_to_stdout("Не удалось прочитать ввод\n");
                break;
            }

            if (bytes_read >= 2 && buffer[0] == 'q' && buffer[1] == '\n') {
                break;
            }

            write(pipe_parent_to_child[WRITE_END], buffer, bytes_read);

            bytes_read = read(pipe_child_to_parent[READ_END], buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                write_to_stdout("Результат: ");
                write_to_stdout(buffer);
                write_to_stdout("\n");
                if (strncmp(buffer, "EXIT", 4) == 0) {
                    write_to_stdout("Обнаружено деление на ноль. Выход.\n");
                    break;
                }
            }
        }

        close(pipe_parent_to_child[WRITE_END]);
        close(pipe_child_to_parent[READ_END]);

        int status;
        waitpid(child_pid, &status, 0);
    }

    return 0;
}