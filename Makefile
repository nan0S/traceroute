## Hubert Obrzut, 309306

TARGET = traceroute
EXENAME = traceroute

CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra
OBJS = $(TARGET).o

all: $(TARGET)

install: $(TARGET) clean

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXENAME) $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o
distclean: clean
	rm -f $(EXENAME)

.PHONY: clean
