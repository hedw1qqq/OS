
all:
	gcc -fsanitize=thread -pthread -o main main.c
	strace -c ./main
