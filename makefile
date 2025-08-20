all: client server
	@echo "Compiled successfully."

client: client.c cJSON.o
	@echo "Compiling client..."
	@gcc -o client client.c cJSON.o

server: server.c cJSON.o
	@echo "Compiling server..."
	@gcc -o server server.c cJSON.o

cJSON.o: cJSON.c
	@echo "Compiling utilities..."
	@gcc -c cJSON.c

clean:
	rm -f *.o main client server