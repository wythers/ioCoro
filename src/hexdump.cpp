#include "hexdump.hpp"

using namespace ioCoro;

void ioCoro::hex_dump(void* p, size_t len, int column = DUMPCOLUMN)
{
        int8_t* buf = static_cast<int8_t*>(p);
        int space = 0;

        printf("hexdump:\n");

        for (uint i = 0; i < len; ++i)
        {
                int l = 0, r = 0;

                __asm__
                (
                        "xorq %%rax, %%rax\n\t"
                        "movb %[buf], %%al\n\t"
                        "shrb $4, %%al\n\t"
                        "movl %%eax, %[l]\n\t"
                        "xorq %%rax, %%rax\n\t"
                        "movb %[buf], %%al\n\t"
                        "shlb $4, %%al\n\t"
                        "shrb $4, %%al\n\t"
                        "movl %%eax, %[r]\n\t"
                        :
                        : [l]"m"(l), [r]"m"(r), [buf]"m"(buf[i])
                        : "%rax"
                );

                printf("%02d%02d%s", l, r, ((space++ % column ) != (column - 1)) || (i == len - 1) ? " " : "\n");
        }

        printf("\n");
}

void ioCoro::dump_regs(void *ucontext)
{
#define FMT "%016llx"

	ucontext_t *uctx = (ucontext_t *)ucontext;
	fprintf(stderr,
		" RAX = 0x" FMT "   RBX = 0x" FMT "   RCX = 0x" FMT "\n"
		" RDX = 0x" FMT "   RSI = 0x" FMT "   RDI = 0x" FMT "\n"
		" RBP = 0x" FMT "   R8  = 0x" FMT "   R9  = 0x" FMT "\n"

		" R10 = 0x" FMT "   R11 = 0x" FMT "   R12 = 0x" FMT "\n"
		" R13 = 0x" FMT "   R14 = 0x" FMT "   R15 = 0x" FMT "\n"
		" RSP = 0x" FMT "\n"
		"\n RIP = 0x" FMT "   EFLAGS = 0x" FMT "\n"

		" TRAP#  = %02lld                ERROR  = %02lld\n"
		/* CR[0,1,3,4] unavailable */
		" CR2 = 0x" FMT "\n"

		, uctx->uc_mcontext.gregs[REG_RAX]
		, uctx->uc_mcontext.gregs[REG_RBX]
		, uctx->uc_mcontext.gregs[REG_RCX]

		, uctx->uc_mcontext.gregs[REG_RDX]
		, uctx->uc_mcontext.gregs[REG_RSI]
		, uctx->uc_mcontext.gregs[REG_RDI]

		, uctx->uc_mcontext.gregs[REG_RBP]
		, uctx->uc_mcontext.gregs[REG_R8]
		, uctx->uc_mcontext.gregs[REG_R9]

		, uctx->uc_mcontext.gregs[REG_R10]
		, uctx->uc_mcontext.gregs[REG_R11]
		, uctx->uc_mcontext.gregs[REG_R12]

		, uctx->uc_mcontext.gregs[REG_R13]
		, uctx->uc_mcontext.gregs[REG_R14]
		, uctx->uc_mcontext.gregs[REG_R15]

		, uctx->uc_mcontext.gregs[REG_RSP]
		, uctx->uc_mcontext.gregs[REG_RIP]
		, uctx->uc_mcontext.gregs[REG_EFL]
		, uctx->uc_mcontext.gregs[REG_TRAPNO]
		, uctx->uc_mcontext.gregs[REG_ERR]
		, uctx->uc_mcontext.gregs[REG_CR2]
		);
}