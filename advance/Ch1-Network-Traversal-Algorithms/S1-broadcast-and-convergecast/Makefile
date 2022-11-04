
######################

all: at launch

launch: launch.cpp payload.o
	g++ -pthread -fcoroutines -latomic -lprotobuf -Wall -std=c++20 $^ -o $@ -liocoro

at: at.cpp payload.o
	g++ -pthread -fcoroutines -latomic -lprotobuf -Wall -std=c++20 $^ -o $@ -liocoro

payload.o: payload.pb.cc
	g++ -lprotobuf -Wall -std=c++20 -c $< -o $@

payload.pb.cc:
	protoc --cpp_out=. payload.proto

clean:
	rm -rf launch at payload.o payload.pb.cc