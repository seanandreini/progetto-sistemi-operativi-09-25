all: exec

exec: client server
	./client ./server

client: client.c
	gcc -c -o client client.c
	chmod +x client

server: server.c
	gcc -c -o server server.c
	chmod +x server

clean:
	rm -f *.o main