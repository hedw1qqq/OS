#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

volatile sig_atomic_t child_finished = 0;

void write_to_stdout(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

// Обработчик для SIGCHLD
void handle_sigchld(int sig) {
    (void)sig; // Чтобы избежать предупреждений о неиспользованной переменной
    int status;
    waitpid(-1, &status, WNOHANG);
    child_finished = 1; // Устанавливаем флаг, чтобы завершить родителя
}

int main() {
    char filename[1024];
    int pipe_parent_to_child[2];
    pid_t child_pid;
    char buffer[1024];

    // Установка обработчика SIGCHLD
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    write_to_stdout("Введите имя файла: ");
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write_to_stdout("Ошибка: не удалось прочитать имя файла\n");
        exit(EXIT_FAILURE);
    }
    filename[bytes_read - 1] = '\0';

    if (pipe(pipe_parent_to_child) == -1) {
        write_to_stdout("Ошибка: не удалось создать трубу\n");
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

        if (execl("./client2", "client2", filename, NULL) == -1) {
            write_to_stdout("Ошибка: не удалось запустить клиентскую программу\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Родительский процесс
        close(pipe_parent_to_child[0]);

        write_to_stdout("Введите числа, разделенные пробелами (или 'q' для выхода):\n");

        while (!child_finished) { // Завершаем, если флаг child_finished установлен
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0 || child_finished) {
                break;
            }
            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "q\n") == 0) {
                write(pipe_parent_to_child[1], "QUIT\n", 5);
                break;
            }

            write(pipe_parent_to_child[1], buffer, bytes_read);
        }

        close(pipe_parent_to_child[1]);
    }

    write_to_stdout("Завершение работы программы.\n");
    return 0;
}