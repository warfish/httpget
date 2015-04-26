CC = gcc
CFLAGS += -std=c99 -Wall -I.

OBJS = url.o httpget.o

all: httpget

httpget: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lpcre -o $@

clean:
	rm -rf *.o httpget
