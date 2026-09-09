CC = gcc
CFLAGS = -Wall -Wextra -std=c11

scheduler: main.c
	$(CC) $(CFLAGS) main.c -o scheduler

clean:
	rm -f scheduler

.PHONY: clean