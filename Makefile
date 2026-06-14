CC = gcc
CFLAGS = -Wall -std=c99 -I./raylib_src/src
LDFLAGS = -L./raylib_src/src -lraylib -lm -lpthread -ldl -lrt -lX11 -lGL

match3_1: match3_1.c
	$(CC) $(CFLAGS) -o match3_1 match3_1.c $(LDFLAGS)

clean:
	rm -f match3_1

run: match3_1
	./match3_1
