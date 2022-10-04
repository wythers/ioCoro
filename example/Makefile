-include ./pattern.mk

MODS := echo http




mods: $(MODS)
	@$(SAY) ***building models...             [start]
	$(call loop,$(MODS),all)
	@$(SAY) ***building models...            [finish]
	
.PHONY: clean
clean-all:
	$(call loop,$(MODS),clean)
