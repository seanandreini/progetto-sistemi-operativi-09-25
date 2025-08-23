all: client server
	@echo "Compiled successfully."

client: src/client.c jsonParser.o cJSON.o functions.o
	@echo "Compiling client..."
	@gcc -g -o bin/client src/client.c build/jsonParser.o build/cJSON.o build/functions.o

server: src/server.c jsonParser.o cJSON.o functions.o
	@echo "Compiling server..."
	@gcc -g -o bin/server src/server.c build/jsonParser.o build/cJSON.o build/functions.o

cJSON.o: lib/cJSON/cJSON.c
	@echo "Compiling cJSON library..."
	@gcc -c -o build/cJSON.o lib/cJSON/cJSON.c

jsonParser.o: src/jsonParser.c cJSON.o
	@echo "Compiling JSON parser..."
	@gcc -c -o build/jsonParser.o src/jsonParser.c

functions.o: src/functions.c
	@echo "Compiling functions..."
	@gcc -c -o build/functions.o src/functions.c

clean:
	rm -f build/*.o bin/client bin/server