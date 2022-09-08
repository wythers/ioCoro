vpath %.hpp ../include
vpath %.cpp ../src

libiocoro.a: objects

objects: reactor.o socket.o iocoro_syscall_impl.o iocoro_syscall.o

iocoro_syscall.o: iocoro_syscall.cpp iocoro_syscall_impl.o socket.o

iocoro_syscall_impl.o: iocoro_syscall_impl.cpp socket.o

socket.o: socket.cpp reactor.o

reactor.o: reactor.cpp


%.a: 
	@echo ***cleaning garbages...            [done]
	@rm -rf ./*.o	

%.o: %.cpp
	@echo cc -c -o $@
	@g++ -pthread -fcoroutines -latomic -I ../include -Wall -std=c++20 -O3 -c -o $@ $^ 
	@ar -r libiocoro.a $@ 2>/dev/null

.PHONY: clean-all
clean-all:
	@echo ***reset source...                 [done]
	@rm -rf ./*.o ./*.a