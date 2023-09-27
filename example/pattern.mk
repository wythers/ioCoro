SAY       := echo
RM        := sudo rm -rf
MAKE      := make
SHELL     := /bin/bash
CC        = g++

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
	@$(CC) -pthread -fcoroutines -latomic -Wall -std=c++20 -fno-rtti -flto -O3 \
	-fdelayed-branch -fif-conversion2 \
	-fmove-loop-invariants -falign-functions -falign-labels -falign-loops -march=native -ffast-math \
	-o $@ $^ -liocoro 2>/dev/null

%.o: %.cpp
	@$(SAY) cc -c -o $@
	@$(CC) -pthread -fcoroutines -latomic -Wall -std=c++20 -fno-rtti -flto -O3 \
	-fdelayed-branch -fif-conversion2 \
	-fmove-loop-invariants -falign-functions -falign-labels -falign-loops -march=native -ffast-math \
	-c -o $@ $^ -liocoro 2>/dev/null