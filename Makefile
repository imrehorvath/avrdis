CC=gcc
CFLAGS=-I. -Wall -O2
DEPS=avrdis.h
OBJECTS=main.o ihexparser.o avrasmgen.o

.PHONY: all clean

all: avrdis

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

avrdis: $(OBJECTS)
	$(CC) -o $@ $^

clean:
	rm -f $(OBJECTS)
