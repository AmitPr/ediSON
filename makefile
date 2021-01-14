.POSIX:
.PHONY: run clean
CC=gcc
# CFLAGS= -g -Wall -Wextra -Wpedantic -Wuninitialized -Wundef -Wcast-align -Wstrict-overflow=2 -Wwrite-strings -Wno-format-nonliteral
BINARY=ediSON
OBJECTS=ediSON.o
# LFLAGS= -lasan

$(BINARY): $(OBJECTS)
	$(CC) -o $(BINARY) $(OBJECTS) $(LFLAGS)

ediSON.o: ediSON.c
	$(CC) $(CFLAGS) -c ediSON.c

run: $(BINARY)
	./$(BINARY)

clean:
	rm *.o $(BINARY)