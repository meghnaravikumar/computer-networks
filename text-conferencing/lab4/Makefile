TARGET = client server

all: ${TARGET}
client: client.c utils.h
	gcc -pthread client.c -o client
server: server.c utils.h
	gcc -pthread server.c -o server
clean:
	rm -f ${TARGET}
