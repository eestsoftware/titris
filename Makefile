PREFIX ?= /usr/local

CC = gcc
CFLAGS = -Wall -Wextra #-Wfaps
DEBUG_CFLAGS += -g3 -O0 -DDEBUG -fno-omit-frame-pointer #debug
BUILD_CFLAGS += -O3 -flto -ffunction-sections -fdata-sections
LDFLAGS = -lm -lncurses -ltinfo

PROGRAM = ./bin/titris
OBJECTS = main.o tetris.o parseargs.o renderer.o

BINDIR:=$(PREFIX)/bin

all: build

build: CFLAGS = -Wall -Wextra $(BUILD_CFLAGS)
build: $(PROGRAM)

$(PROGRAM): $(OBJECTS) | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: ./src/%.c
	$(CC) $(CFLAGS) -c $<

debug: CFLAGS = -Wall -Wextra $(DEBUG_CFLAGS)
debug: clean $(PROGRAM)

install: ./bin/titris
	install -d $(BINDIR)
	install -s -Dm755 ./bin/titris -t $(BINDIR)

uninstall:
	rm ${BINDIR}/titris

clean:
	rm -f $(OBJECTS) $(PROGRAM)
