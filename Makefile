# @(#)Makefile  2024-08-04  A.J.Travis

#
# make THC programs and utilities
#

INSTALL = /usr/local/bin
SYSTEM = /usr/local/sbin
BIN = thc stamp
LINK = thcc
SCRIPT = thc-format dups fci fco fdiff rmdups
ADMIN = flocate
CFLAGS = -O -l xxhash

all: $(BIN) $(LINK) $(SCRIPT)

thc: thc.c
	$(CC) -o $@ thc.c $(CFLAGS)

thcc:	thc
	ln -s $< $@
	
stamp: stamp.c
	$(CC) -o $@ stamp.c

install: all
	mkdir -p $(INSTALL)
	install -p -o root -g root $(BIN) $(INSTALL)
	install -p -o root -g root $(SCRIPT) $(INSTALL)
	install -p -o root -g root $(ADMIN) $(SYSTEM)
	cp -a $(LINK) $(INSTALL)

xxh3_demo: xxh3_demo.c
	cc -o $@ $< $(CFLAGS)

xxh64_demo: xxh64_demo.c
	cc -o $@ $< $(CFLAGS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(BIN)
