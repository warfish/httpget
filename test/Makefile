CC = gcc
CFLAGS += -std=c99 -Wall -I. -I..

OBJS = t_url.o ../url.o
TARGET = urltest

all: $(TARGET) Makefile

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lcunit -lpcre -o $@

clean:
	rm -rf *.o ../*.o $(TARGET)
