
.POSIX:

CC = cc -std=c89
CFLAGS = -s -Wall -Wextra -Os -g3
LDFLAGS =
LDLIBS = # -lm
PREFIX = /usr/local

CFLAGS += -D_POSIX_C_SOURCE=2

all: shpdump endian

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f bin/shpdump $(DESTDIR)$(PREFIX)/bin
#	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
#	gzip < shpdump.1 > $(DESTDIR)$(PREFIX)/share/man/man1/shpdump.1.gz

shpdump: bin/shpdump
endian: bin/endian

bin/shpdump: obj/shpdump.o obj/endian.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

bin/endian: src/endian.c src/endian.h
	$(CC) $(CFLAGS) -DTEST -o $@ $^

DEPS = src/shapefile.h src/endian.h
obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f bin/* obj/*.o

tgz: clean
	(cd ..; tar chzvf shpdump-`date +%Y%m%d`.tgz \
	--exclude RCS --exclude aux shpdump)

.PHONY: all install clean tgz
