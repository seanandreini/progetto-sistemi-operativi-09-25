# MAKEFLAGS += -j

# all: exec
all: client server
	@echo "Compiled successfully."

exec:
	./client & ./server

client: client.c
	@echo "Compiling client..."
	@gcc -o client client.c

server: server.c
	@echo "Compiling server..."
	@gcc -o server server.c

clean:
	rm -f *.o main client server