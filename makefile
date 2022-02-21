all: server.c
	gcc -Wall -o server server.c
remove:
	rm server
