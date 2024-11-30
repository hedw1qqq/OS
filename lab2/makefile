
all:
	gcc -fsanitize=thread -pthread -lm -o main main.c
	strace -c ./main 10
