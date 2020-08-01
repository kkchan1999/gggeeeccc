CC = gcc
CFLAGS = -O0 -Wall -g -lpthread

test:main.c pool.c
	$(CC) $^ -o $@ $(CFLAGS)

debug:main.c pool.c
	$(CC) $^ -o $@ $(CFLAGS) -DDEBUG

clean:
	$(RM) .*.sw? test debug *.o

.PHONY:all clean