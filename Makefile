CC = gcc
CFLAGS = -I. -Wall -O2
DEPS = avrdis.h
OBJECTS = main.o avrdis.o ihexparser.o avrasmgen.o
PREFIX ?= /usr/local

.PHONY: all clean install

all: avrdis

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

avrdis: $(OBJECTS)
	$(CC) -o $@ $^

clean:
	rm -f $(OBJECTS) avrdis

install: all
	install -m 0775 -d $(DESTDIR)$(PREFIX)/bin/
	install avrdis $(DESTDIR)$(PREFIX)/bin/
