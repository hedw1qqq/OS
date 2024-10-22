#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
int is_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    if (*str == '-') {
        str++;
    }
    while (*str != '\0') {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

int safe_divide(int a, int b, int *error) {
    if (b == 0) {
        *error = 1;
        return 0;
    }

    *error = 0;
    return a / b;
}

void write_to_stdout(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void write_to_file(int fd, const char *message) {
    write(fd, message, strlen(message));
}

int main(int argc, char **argv) {
    int end_program = 0;
    if (argc != 2) {
        write_to_stdout("Ошибка: не передано имя файла\n");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    const char* filename = argv[1];
    int output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (output_fd == -1) {
        write_to_stdout("Не удалось открыть выходной файл\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if(end_program){
            break;
        }
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            break;
        }

        buffer[bytes_read] = '\0';

        if (strcmp(buffer, "QUIT\n") == 0) {
            break;
        }

        char *token;
        int numbers[1024];
        int count = 0;
        int invalid_input = 0;

        token = strtok(buffer, " \n");
        while (token != NULL) {
            if (is_number(token)) {
                long val = strtol(token, NULL, 10);
                if (val > INT_MAX || val < INT_MIN) {
                    invalid_input = 1;
                    break;
                }
                numbers[count++] = (int)val;
            } else {
                invalid_input = 1;
                break;
            }
            token = strtok(NULL, " \n");
        }

        if (invalid_input) {
            write_to_file(output_fd, "Ошибка: введено некорректное число\n");
            continue;
        }

        if (count < 2) {
            write_to_file(output_fd, "Ошибка: необходимо как минимум два числа\n");
            continue;
        }

        int result = numbers[0];
        int error = 0;
        for (int i = 1; i < count; i++) {
            result = safe_divide(result, numbers[i], &error);
            if (error == 1) {
                end_program = 1;
                write_to_file(output_fd, "Ошибка: Обнаружено деление на ноль\n");
                close(output_fd);
                exit(EXIT_FAILURE);  // Завершаем дочерний процесс при делении на ноль
            }
        }

        char result_buffer[50];
        snprintf(result_buffer, sizeof(result_buffer), "Результат: %d\n", result);
        write_to_file(output_fd, result_buffer);
    }

    close(output_fd);
    return 0;
}