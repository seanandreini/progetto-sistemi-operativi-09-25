all: client server
	@echo "Compiled successfully."

client: client.c jsonParser.o cJSON.o functions.o
	@echo "Compiling client..."
	@gcc -o client client.c jsonParser.o cJSON.o functions.o

server: server.c jsonParser.o cJSON.o functions.o
	@echo "Compiling server..."
	@gcc -o server server.c jsonParser.o cJSON.o functions.o

cJSON.o: cJSON.c
	@echo "Compiling cJSON library..."
	@gcc -c cJSON.c

jsonParser.o: jsonParser.c cJSON.o
	@echo "Compiling JSON parser..."
	@gcc -c jsonParser.c

functions.o: functions.c
	@echo "Compiling functions..."
	@gcc -c functions.c

clean:
	rm -f *.o main client server