all: client server
	@echo "Compiled successfully."

client: ./src/client.c jsonParser.o cJSON.o functions.o bin
	@echo "Compiling client..."
	@gcc -g -o bin/client ./src/client.c ./build/jsonParser.o ./build/cJSON.o ./build/functions.o

server: ./src/server.c jsonParser.o cJSON.o functions.o bin
	@echo "Compiling server..."
	@gcc -g -o bin/server ./src/server.c ./build/jsonParser.o ./build/cJSON.o ./build/functions.o

bin:
	@echo "Creating ./bin..."
	@mkdir -p ./bin

build:
	@echo "Creating ./build..."
	@mkdir -p ./build

cJSON.o: lib/cJSON/cJSON.c build
	@echo "Compiling cJSON library..."
	@gcc -c -o ./build/cJSON.o lib/cJSON/cJSON.c

jsonParser.o: ./src/jsonParser.c cJSON.o build
	@echo "Compiling JSON parser..."
	@gcc -c -o ./build/jsonParser.o ./src/jsonParser.c

functions.o: ./src/functions.c build
	@echo "Compiling functions..."
	@gcc -c -o ./build/functions.o ./src/functions.c

clean:
	rm -f ./build/*.o bin/client bin/server