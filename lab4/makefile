CC=gcc
CFLAGS=-fPIC
LDFLAGS=-shared

all: free_list.so mckusick.so main

free_list.so: free_list.c allocator.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

mckusick.so: mckusick.c allocator.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

main: main.c allocator.h
	$(CC) $(CFLAGS) -o $@ $< -ldl

clean:
	rm -f *.so main
