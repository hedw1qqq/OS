all: allocator_list.so allocator_mc.so main

allocator_list.so: allocator_list.c
	gcc -shared -o allocator_list.so allocator_list.c -fPIC

allocator_mc.so: allocator_mc.c
	gcc -shared -o allocator_mc.so allocator_mc.c -fPIC

main: main.c
	gcc -o main main.c -ldl

clean:
	rm -f main allocator_list.so allocator_mc.so
