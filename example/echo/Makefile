ifneq "$(MAKECMDGOALS)" "clean"
	-include ../pattern.mk
endif

all: echo_client echo_server

echo_client: echo_client.cpp 

echo_server: echo_server.cpp

.PHONY: clean
clean:
	@$(RM) echo_client echo_server
