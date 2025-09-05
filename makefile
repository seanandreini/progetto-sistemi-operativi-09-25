src_dir=./src
bin_dir=./bin
build_dir=./build
lib_dir=./lib

all: client server
	@echo "Compiled successfully."

client: $(src_dir)/client.c jsonParser.o cJSON.o functions.o bin_folder
	@echo "Compiling client..."
	@gcc -g -o $(bin_dir)/client $(src_dir)/client.c $(build_dir)/jsonParser.o $(build_dir)/cJSON.o $(build_dir)/functions.o

server: $(src_dir)/server.c jsonParser.o cJSON.o functions.o bin_folder
	@echo "Compiling server..."
	@gcc -g -o $(bin_dir)/server $(src_dir)/server.c $(build_dir)/jsonParser.o $(build_dir)/cJSON.o $(build_dir)/functions.o

bin_folder:
	@echo "Creating $(bin_dir)..."
	@mkdir -p $(bin_dir)

build_folder:
	@echo "Creating $(build_dir)..."
	@mkdir -p $(build_dir)

cJSON.o: $(lib_dir)/cJSON/cJSON.c build_folder
	@echo "Compiling cJSON library..."
	@gcc -c -o $(build_dir)/cJSON.o $(lib_dir)/cJSON/cJSON.c

jsonParser.o: $(src_dir)/jsonParser.c cJSON.o build_folder
	@echo "Compiling JSON parser..."
	@gcc -c -o $(build_dir)/jsonParser.o $(src_dir)/jsonParser.c

functions.o: $(src_dir)/functions.c build_folder
	@echo "Compiling functions..."
	@gcc -c -o $(build_dir)/functions.o $(src_dir)/functions.c

clean:
	@echo "Removing objects and executables..."
	@rm -f $(build_dir)/*.o $(bin_dir)/server $(bin_dir)/client