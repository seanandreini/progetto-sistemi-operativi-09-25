all: client server
	@echo "Compiled successfully."

client: client.c jsonParser.o
	@echo "Compiling client..."
	@gcc -o client client.c cJSON.o

server: server.c jsonParser.o
	@echo "Compiling server..."
	@gcc -o server server.c cJSON.o

cJSON.o: cJSON.c
	@echo "Compiling cJSON library..."
	@gcc -c cJSON.c

jsonParser.o: jsonParser.c cJSON.o
	@echo "Compiling JSON parser..."
	@gcc -c jsonParser.c

clean:
	rm -f *.o main client server