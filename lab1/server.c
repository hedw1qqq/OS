#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

void write_to_stdout(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

int main() {
    char filename[1024];
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];
    pid_t child_pid;
    char buffer[1024];

    write_to_stdout("Введите имя файла: ");
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write_to_stdout("Ошибка: не удалось прочитать имя файла\n");
        exit(EXIT_FAILURE);
    }
    filename[bytes_read - 1] = '\0';

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        write_to_stdout("Ошибка: не удалось создать трубы\n");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();

    if (child_pid == -1) {
        write_to_stdout("Ошибка: не удалось создать дочерний процесс\n");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Дочерний процесс
        close(pipe_parent_to_child[1]);
        dup2(pipe_parent_to_child[0], STDIN_FILENO);
        close(pipe_parent_to_child[0]);

        close(pipe_child_to_parent[0]);
        dup2(pipe_child_to_parent[1], STDERR_FILENO);
        close(pipe_child_to_parent[1]);

        if (execl("./client", "client", filename, NULL) == -1) {
            write_to_stdout("Ошибка: не удалось запустить клиентскую программу\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Родительский процесс
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        write_to_stdout("Введите числа, разделенные пробелами (или 'q' для выхода):\n");

        while (1) {
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                break;
            }
            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "q\n") == 0) {
                write(pipe_parent_to_child[1], "QUIT\n", 5);
                break;
            }

            // Отправляем данные клиенту
            write(pipe_parent_to_child[1], buffer, bytes_read);

            // Читаем ответ от клиента
            char response[1024];
            ssize_t response_size = read(pipe_child_to_parent[0], response, sizeof(response) - 1);
            if (response_size > 0) {
                response[response_size] = '\0';
                write_to_stdout(response);

                // Проверяем на EXIT
                if (strstr(response, "EXIT") != NULL) {
                    break;
                }
            }
        }

        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);
        int status;
        waitpid(child_pid, &status, 0);
    }
    return 0;
}