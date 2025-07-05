.PHONY: all clean install

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: weston-deferred.bin

weston-deferred.bin: weston-deferred.cpp
	g++ -std=c++20 -o $@ $< -lsystemd

clean:
	rm -f *.bin *.o

install: weston-deferred.bin
	install -Dm755 weston-deferred.bin $(BINDIR)/weston-deferred
