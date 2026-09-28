CC      = gcc
CFLAGS  = -O2 -Wall -Wextra 

all: rebus_v1 rebus_v2 rebus_v3

rebus_v1: rebus_v1.c
	$(CC) $(CFLAGS) -o rebus_v1 rebus_v1.c

rebus_v2: rebus_v2.c
	$(CC) $(CFLAGS) -o rebus_v2 rebus_v2.c

rebus_v3: rebus_v3.c
	$(CC) $(CFLAGS) -o rebus_v3 rebus_v3.c

run: all
	./rebus_v1
	./rebus_v2
	./rebus_v3

clean:
	rm -f rebus_v1 rebus_v2 rebus_v3

.PHONY: all run clean
