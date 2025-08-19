MAKEFLAGS += -j

all: exec

exec:
	./client & ./server

client: client.c
	gcc -o client client.c
	chmod +x client

server: server.c
	gcc -o server server.c
	chmod +x server

clean:
	rm -f *.o main client server