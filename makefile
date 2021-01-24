.POSIX:
.PHONY: run clean
CC=gcc
CFLAGS= -g -Wall -Wextra -Wpedantic -Wuninitialized -Wundef -Wcast-align -Wstrict-overflow=2 -Wwrite-strings -Wno-format-nonliteral
BINARY=ediSON 
OBJECTS=ediSON.o JSONTokenizer.o utils.o
LFLAGS= -lasan
ARGS=ediSON.c

$(BINARY): $(OBJECTS)
	$(CC) -o $(BINARY) $(OBJECTS) $(LFLAGS)

ediSON.o: ediSON.c JSONTokenizer.o utils.o
	$(CC) $(CFLAGS) -c ediSON.c

JSONTokenizer.o: JSONTokenizer.c utils.o
	$(CC) $(CFLAGS) -c JSONTokenizer.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

run: $(BINARY)
	./$(BINARY) $(ARGS)

clean:
	rm *.o $(BINARY)
