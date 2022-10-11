ifneq "$(MAKECMDGOALS)" "clean"
	-include ../pattern.mk
endif

all: asker answerer

asker: asker.cpp

answerer: answerer.cpp

.PHONY: clean
clean:
	@$(RM) asker answerer