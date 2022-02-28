all: server.c
	gcc -Wall -o server server.c
clear:
	rm server
