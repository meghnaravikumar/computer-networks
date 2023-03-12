TARGET = client server

all: ${TARGET}
client: client.c message.h client_utils.h server_utils.h
	gcc client.c -o client
server: server.c message.h client_utils.h server_utils.h
	gcc server.c -o server
clean:
	rm -f ${TARGET}
