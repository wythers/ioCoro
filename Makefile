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


build:
	@$(SAY) ***building iocoro library...                    [start]
	@$(MAKE) -s --directory=./lib libiocoro.a
	@$(SAY) ***building iocoro library...                    [finish]
	

	@$(SAY) ***building environment...                       [start]
	@$(MKDIR) $(IENV)/iocoro
	@$(LINKER) $(HOME)/include/* $(IENV)/iocoro
	@$(LINKER) $(HOME)/lib/libiocoro.a $(LENV)
	@$(UPDATELIB)
	@$(SAY) ***building environment...                       [finish]

	@$(MAKE) -s --directory=example mods
	
	@$(SAY) --- All successfully, use -liocoro, as a g++ arg, to linker iocoro library...


.PHONY: clean
clean:
	@$(MAKE) -s --directory=lib clean-all
	@$(MAKE) -s --directory=example clean-all
	@$(SAY) ***restore default environment...                [start]
	@$(RM) /usr/local/include/iocoro
	@$(RM) /usr/local/lib/libiocoro.a
	@$(SAY) ***restore default environment...                [finish]

	

