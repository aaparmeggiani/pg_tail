
INC=-I`pg_config --includedir`
LIB=-L`pg_config --libdir`
INSTALL=/usr/local/bin

CFLAGS=-Wall -O2
LDFLAGS=-lpq

all: pg_tail

pg_tail:
	$(CC) $(CFLAGS) $(INC) $(LIB) $(LDFLAGS) -o pg_tail pg_tail.c

install: pg_tail
	cp pg_tail $(INSTALL)

clean:
	rm -f pg_tail

