SAY       := echo
RM        := sudo rm -rf
MAKE      := make
SHELL     := /bin/bash

define loop
	@for m in $1;                                         \
	do                                                    \
		$(MAKE) -s --directory=$$m $2;                \
	done
endef

%: %.cpp
	@$(SAY) cc -o $@
	@g++ -pthread -fcoroutines -latomic -Wall -std=c++20 -O3 -o $@ $^ -liocoro