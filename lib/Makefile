ifneq "$(MAKECMDGOALS)" "clean"
	-include ./pattern.mk
endif

lib         := libiocoro.a
level-one   := socket_ops.o random.o reactor.o hexdump.o
level-two   := socket.o 
level-three := iocoro_syscall_impl.o
level-four  := iocoro_syscall.o

# lib target...
$(lib): $(level-four)

$(call building,$(level-one))
$(call building,$(level-two),$(level-one))
$(call building,$(level-three),$(level-two))
$(call building,$(level-four),$(level-three))


# clean target
.PHONY: clean-all
clean-all:
	@$(SAY) ***reset source...                 [done]
	@$(RM) ./*.o ./*.a

