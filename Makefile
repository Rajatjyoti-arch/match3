CC = gcc
CFLAGS = -Wall -std=c99 -I./raylib_src/src
LDFLAGS = -L./raylib_src/src -lraylib -lm -lpthread -ldl -lrt -lX11 -lGL

match3: match3.c
	$(CC) $(CFLAGS) -o match3 match3.c $(LDFLAGS)

clean:
	rm -f match3

run: match3
	./match3
