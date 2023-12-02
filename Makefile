CC=gcc
CFLAGS=-Wall -Wextra -Werror -lresolv

all: stress-ntp

stress-ntp: stress-ntp.o
	$(CC) $(CFLAGS) -o stress-ntp stress-ntp.o

stress-ntp.o: stress-ntp.c stress-ntp.h
	$(CC) $(CFLAGS) -c stress-ntp.c

clean:
	rm -f stress-ntp stress-ntp.o

.PHONY: all clean
