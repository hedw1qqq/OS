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
    pid_t child_pid;
    char buffer[1024];

    write_to_stdout("Введите имя файла: ");
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write_to_stdout("Ошибка: не удалось прочитать имя файла\n");
        exit(EXIT_FAILURE);
    }
    filename[bytes_read - 1] = '\0';
    if (pipe(pipe_parent_to_child) == -1) {
        write_to_stdout("Ошибка: не удалось создать трубы\n");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();

    if (child_pid == -1) {
        write_to_stdout("Ошибка: не удалось создать дочерний процесс\n");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        close(pipe_parent_to_child[1]);
        dup2(pipe_parent_to_child[0], STDIN_FILENO);
        close(pipe_parent_to_child[0]);

        if (execl("./client", "client", filename, NULL) == -1) {
            write_to_stdout("Ошибка: не удалось запустить клиентскую программу\n");
            exit(EXIT_FAILURE);
        }
    } else {
        close(pipe_parent_to_child[0]);

        write_to_stdout("Введите числа, разделенные пробелами (или 'q' для выхода):\n");

        while (1) {
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                break;
            }

            if (bytes_read == 2 && buffer[0] == 'q' && buffer[1] == '\n') {
                write(pipe_parent_to_child[1], "QUIT\n", 5);
                break;
            }

            write(pipe_parent_to_child[1], buffer, bytes_read);

            // Проверяем, завершился ли дочерний процесс
            int status;
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            if (result == child_pid) {
                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) == EXIT_FAILURE) {
                        write_to_stdout("Дочерний процесс завершен из-за ошибки\n");
                    } else {
                        write_to_stdout("Дочерний процесс завершен\n");
                    }
                    break;
                }
            }
        }

        close(pipe_parent_to_child[1]);
        waitpid(child_pid, NULL, 0);
    }

    return 0;
}