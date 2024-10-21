
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#define BUFFER_SIZE 1024

int is_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    if (*str == '-') {
        str++;
    }

    while (*str != '\0') {
        if (!isdigit((unsigned char)*str)) {
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
    
    if (a == INT_MIN && b == -1) {
        *error = 2; 
        return 0;
    }
    
    int result = a / b;
    
 
    if (a != 0 && result != 0 && (a ^ result) < 0 && (a ^ b) < 0) {
        *error = 2; 
        return 0;
    }
    
    *error = 0; 
    return result;
}

int main() {
    char buffer[BUFFER_SIZE];
    char filename[BUFFER_SIZE];

    if (fgets(filename, BUFFER_SIZE, stdin) == NULL) {
        fprintf(stderr, "Ошибка при чтении имени файла\n");
        exit(EXIT_FAILURE);
    }
    filename[strcspn(filename, "\n")] = 0;

    FILE *output_file = fopen(filename, "w");
    if (output_file == NULL) {
        perror("Не удалось открыть выходной файл");
        exit(EXIT_FAILURE);
    }

    char result_buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }

        buffer[bytes_read] = '\0';

        char *token;
        int numbers[BUFFER_SIZE];
        int count = 0;
        int invalid_input = 0;

        token = strtok(buffer, " \n");
        while (token != NULL && count < BUFFER_SIZE) {
            if (is_number(token)) {
                long val = strtol(token, NULL, 10);
                if (((val == LONG_MAX || val == LONG_MIN)) || 
                   
                    val > INT_MAX || val < INT_MIN) {
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
            fprintf(output_file, "Ошибка: введено некорректное число\n");
            fprintf(stdout, "Ошибка: введено некорректное число\n");
            fflush(stdout);
            fflush(output_file);
            continue;
        }

        if (count < 2) {
            fprintf(output_file, "Некорректный ввод: необходимо как минимум два числа\n");
            fprintf(stdout, "Ошибка: необходимо как минимум два числа\n");
            fflush(stdout);
            fflush(output_file);
            continue;
        }

        int result = numbers[0];
        int error = 0;

        for (int i = 1; i < count; i++) {
            result = safe_divide(result, numbers[i], &error);
            if (error != 0) {
                break;
            }
        }

        if (error == 1) {
            fprintf(output_file, "Обнаружено деление на ноль\n");
            fprintf(stdout, "EXIT\n");
            fflush(stdout);
            fflush(output_file);
            break;
        } else if (error == 2) {
            fprintf(output_file, "Произошло переполнение при вычислении\n");
            fprintf(stdout, "Ошибка: переполнение\n");
            fflush(stdout);
            fflush(output_file);
            continue;
        } else {
            fprintf(output_file, "Результат: %d\n", result);
            fprintf(stdout, "%d\n", result);
            fflush(stdout);
            fflush(output_file);
        }
    }

    fclose(output_file);
    return 0;
}