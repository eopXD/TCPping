CC = gcc
DEL = rm -f
all:
		$(CC) client.c -c
		$(CC) client.o -o client
		$(CC) server.c -c
		$(CC) server.o -o server
		$(DEL) *.o
		$(DEL) *~
clean:
		$(DEL) client
		$(DEL) server