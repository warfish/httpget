CC = gcc
CFLAGS += -std=c99 -Wall -I.

OBJS = url.o httpget.o
TEST_OBJS = url.o test/t_url.o

all: httpget

httpget: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lpcre -o $@

urltest: $(TEST_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TEST_OBJS) -lcunit -lpcre -o $@	

clean:
	rm -rf *.o ./test/*.o httpget urltest
