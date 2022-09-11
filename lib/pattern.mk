vpath %.hpp ../include
vpath %.cpp ../src

SAY       := echo
RM        := rm -rf
MAKE      := make
SHELL     := /bin/bash
CC        := g++
AR        := ar

# tariget: .cpp
define target
$1: $(subst .o,.cpp,$1) $2
endef

# create null static lib
define fexist
	@if [ ! -e $(lib) ];                 \
	then                                 \
		ar -r $(lib) 2>/dev/null;    \
	fi      
endef 

# building handler
define building
$(foreach p,$1,$(eval $(call target,$p,$2)))
endef


%.a: 
	@$(SAY) ***cleaning garbages...            [done]
	@$(RM) ./*.o	

%.o: %.cpp
	@$(SAY) cc -c -o $@
	$(fexist)
	@$(CC) -pthread -fcoroutines -latomic -I ../include -Wall -std=c++20 -O3 -c -o $@ $< $(lib) 2>/dev/null
	@$(AR) -r $(lib) $@ 2>/dev/null

