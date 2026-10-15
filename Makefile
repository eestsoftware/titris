CC = clang
CFLAGS = -Wall -O3
LDFLAGS = -lm -lncurses -ltinfo

PROGRAM = ./bin/titris
OBJECTS = main.o tetris.o

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: ./src/main.c
	$(CC) $(CFLAGS) -c $<

tetris.o: ./src/tetris.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(PROGRAM)
