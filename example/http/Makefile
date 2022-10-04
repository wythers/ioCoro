ifneq "$(MAKECMDGOALS)" "clean"
	-include ../pattern.mk
endif

HTTP := http_client http_server

OBJS := http_chunk.o http_service.o

all: $(HTTP)

$(call building,http_client,$(OBJS))
$(call building,http_server,$(OBJS)) 

http_server.o: http_service.cpp http_chunk.o

http_chunk.o: http_chunk.cpp


clean:
	@rm -rf $(HTTP) $(OBJS)

