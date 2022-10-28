

##############################

main: main.cpp vertex_info.o
	g++ -pthread -fcoroutines -latomic -lprotobuf -Wall -std=c++20 $^ -o $@ -liocoro

vertex_info.o: vertex_info.pb.cc
	g++ -lprotobuf -Wall -std=c++20 -c $< -o $@

vertex_info.pb.cc:
	protoc --cpp_out=. vertex_info.proto

clean:
	rm -rf main vertex_info.pb.cc vertex_info.pb.h vertex_info.o