SHELL     := /bin/bash
MAKE      := make
DIR       := lib example
UPDATELIB := sudo ldconfig
ARGS      := 
LINKER    := sudo ln -s
SAY       := echo
RM        := sudo rm -rf
MKDIR     := sudo mkdir

IENV      := /usr/local/include
LENV      := /usr/local/lib
HOME 	  := $(shell pwd)

ARG1 =

build:
	@$(SAY) ***building iocoro library...                    [start]
ifeq "$(ARG1)" ""
	@$(MAKE) -s --directory=./lib libiocoro.a
else
	@$(MAKE) -s --directory=./lib CC=$(ARG1) libiocoro.a 
	@$(SAY) ***building iocoro library...                    [finish]
endif

	@$(SAY) ***building environment...                       [start]
	@$(MKDIR) $(IENV)/iocoro
	@$(LINKER) $(HOME)/include/* $(IENV)/iocoro
	@$(LINKER) $(HOME)/lib/libiocoro.a $(LENV)
	@$(UPDATELIB)
	@$(SAY) ***building environment...                       [finish]

ifeq "$(ARG1)" ""
	@$(MAKE) -s --directory=example mods
else
	@$(MAKE) -s --directory=example CC=$(ARG1) mods
endif
	
	@$(SAY) --- All successfully, use -liocoro, as a g++ arg, to linker iocoro library...


.PHONY: clean
clean:
	@$(MAKE) -s --directory=lib clean-all
	@$(MAKE) -s --directory=example clean-all
	@$(SAY) ***restore default environment...                [start]
	@$(RM) $(IENV)/iocoro
	@$(RM) $(LENV)/libiocoro.a
	@$(SAY) ***restore default environment...                [finish]

	

