ifneq "$(MAKECMDGOALS)" "clean"
	-include ../pattern.mk
endif

all: feet brain

feet: feet.cpp

brain: brain.cpp

.PHONY: clean
clean:
	@$(RM) feet brain