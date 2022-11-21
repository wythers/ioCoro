
######################

all: tree vertex

tree: tree.cpp payload.o
	@echo cc -o tree
	@g++ -I ../../ -pthread -fcoroutines -latomic -lprotobuf -Wall -std=c++20 $^ -o $@ -liocoro

vertex: vertex.cpp payload.o
	@echo cc -o vertex
	@g++ -I ../../ -pthread -fcoroutines -latomic -lprotobuf -Wall -std=c++20 $^ -o $@ -liocoro

payload.o: payload.pb.cc
	@echo cc -c -o payload.o
	@g++ -lprotobuf -Wall -std=c++20 -c $< -o $@

payload.pb.cc:
	protoc --cpp_out=. payload.proto

clean:
	@rm -rf tree vertex payload.o payload.pb.cc payload.pb.h