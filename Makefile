#
# Makefile for lab 2
#

CFLAGS = -ggdb3 -Wall -pedantic -g -fstack-protector-all -fsanitize=address
#shell36: shell36.c parser.c
#	gcc shell36.c parser.c -o shell36 $(CFLAGS)

shell36: shell36.c parser.c
	gcc $(shell python3-config --cflags) shell36.c parser.c -o shell36 $(shell python3-config --ldflags) -L/usr/lib/python3.12/config-3.12-x86_64-linux-gnu -lpython3.12 -lpthread -ldl -lutil -lm


clean:
	rm -f *.o shell36
