ifneq "$(MAKECMDGOALS)" "clean"
	-include ../pattern.mk
endif

all: test_client test_server

test_client: test_client.cpp

test_server:test_server.cpp

.PHONY: clean
clean:
	@$(RM) test_client test_server