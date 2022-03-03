all: server.c
	gcc -Wall -o server server.c -lm
clear:
	rm server
