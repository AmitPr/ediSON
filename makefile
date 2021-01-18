.POSIX:
.PHONY: run clean
CC=gcc
# CFLAGS= -g -Wall -Wextra -Wpedantic -Wuninitialized -Wundef -Wcast-align -Wstrict-overflow=2 -Wwrite-strings -Wno-format-nonliteral
BINARY=ediSON 
OBJECTS=ediSON.o JSONObjects.o utils.o
# LFLAGS= -lasan

$(BINARY): $(OBJECTS)
	$(CC) -o $(BINARY) $(OBJECTS) $(LFLAGS)

ediSON.o: ediSON.c JSONObjects.o utils.o
	$(CC) $(CFLAGS) -c ediSON.c
JSONObjects.o: JSONObjects.c utils.o
	$(CC) $(CFLAGS) -c JSONObjects.c
utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

run: $(BINARY)
	./$(BINARY)

clean:
	rm *.o $(BINARY)