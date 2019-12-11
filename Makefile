pgc := $(shell which pg_config)
inc := $(shell $(pgc) --includedir)
lib := $(shell $(pgc) --libdir)

INSTALL=/usr/local/bin
INC=-I$(inc) 
LIB=-L$(lib)
CFLAGS=-Wall -O2
LDFLAGS=-lpq

clean:
	rm -f pg_tail

pg_tail: pg_tail.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o pg_tail pg_tail.c $(LDFLAGS)

all: pg_tail

install: pg_tail
	cp pg_tail $(INSTALL)

uninstall:
	rm $(INSTALL)/pg_tail

homebrew:
	cp pg_tail $(PREFIX)



