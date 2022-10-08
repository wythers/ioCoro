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

define obj-target
$1: $(subst .o,.cpp,$1)
endef

define building-objs
$(foreach f,$1,$(call obj-target,f))
endef

define building
$1: $(addsuffix .cpp,$1) $2
endef

define building-obj
$1: $(addsuffix .o,$1) $2
endef

%: %.cpp
	@$(SAY) cc -o $@
	@g++ -pthread -fcoroutines -latomic -Wall -std=c++20 -O3 -o $@ $^ -liocoro 2>/dev/null

%.o: %.cpp
	@$(SAY) cc -c -o $@
	@g++ -pthread -fcoroutines -latomic -Wall -std=c++20 -O3 -c -o $@ $^ -liocoro 2>/dev/null