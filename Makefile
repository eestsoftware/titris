CC = gcc
CFLAGS = -Wall -Wextra #-Wfaps
CFLAGS += -g3 -O0 -DDEBUG -fno-omit-frame-pointer #debug
#CFLAGS += -Os
LDFLAGS = -lm -lncurses -ltinfo

PROGRAM = ./bin/titris
OBJECTS = main.o tetris.o parseargs.o

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: ./src/main.c
	$(CC) $(CFLAGS) -c $<

tetris.o: ./src/tetris.c
	$(CC) $(CFLAGS) -c $<

parseargs.o: ./src/parseargs.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(PROGRAM)
