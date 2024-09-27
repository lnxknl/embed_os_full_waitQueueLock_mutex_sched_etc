/**
 * @file
 * @brief
 *
 * @author Aleksey Zhmulin
 * @date 21.08.23
 */
#include <assert.h>
#include <stdint.h>

#include <asm/regs.h>
#include <riscv/exception.h>
#include <debug/breakpoint.h>

static bpt_handler_t __bpt_handler;

static int riscv_bpt_excpt_handler(long unsigned nr, void *data) {
	assert(__bpt_handler);

	__bpt_handler(data);

	return 0;
}

void bpt_handler_set(bpt_handler_t handler) {
	__bpt_handler = handler;

	if (handler) {
		riscv_excpt_table[EXC_BREAKPOINT] = riscv_bpt_excpt_handler;
	}
	else {
		riscv_excpt_table[EXC_BREAKPOINT] = NULL;
	}
}
