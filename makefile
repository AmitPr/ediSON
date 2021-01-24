.POSIX:
.PHONY: run clean
CC=gcc
CFLAGS= -g -Wall -Wextra -Wpedantic -Wuninitialized -Wundef -Wcast-align -Wstrict-overflow=2 -Wwrite-strings -Wno-format-nonliteral
BINARY=ediSON 
OBJECTS=editor.o JSONTokenizer.o utils.o
LFLAGS= -lasan

$(BINARY): $(OBJECTS)
	$(CC) -o $(BINARY) $(OBJECTS) $(LFLAGS)

editor.o: editor.c JSONTokenizer.o utils.o
	$(CC) $(CFLAGS) -c editor.c
JSONTokenizer.o: JSONTokenizer.c utils.o
	$(CC) $(CFLAGS) -c JSONTokenizer.c
utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

run: $(BINARY)
	./$(BINARY)

clean:
	rm *.o $(BINARY)