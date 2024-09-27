/**
 * @file
 * @brief
 *
 * @author Aleksey Zhmulin
 * @date 06.11.22
 */

#include <inttypes.h>
#include <stdint.h>

#include <arm/exception.h>
#include <hal/reg.h>
#include <hal/test/traps_core.h>
#include <util/log.h>

fault_handler_t arm_data_fault_table[0x10];

void arm_dabt_handler(excpt_context_t *ctx) {
	uint32_t fault_status;

	fault_status = ARCH_REG_LOAD(DFSR) & 0xf;

	if (arm_data_fault_table[fault_status]) {
		arm_data_fault_table[fault_status](fault_status, ctx);
	}
	else {
		log_raw(LOG_EMERG, "\nUnresolvable data abort exception!\n");
		log_raw(LOG_EMERG, "Fault status = %" PRIu32 "\n", fault_status);
		log_raw(LOG_EMERG, "Fault address = %#" PRIx32 "\n", ARCH_REG_LOAD(DFAR));
		arm_show_excpt_context(ctx);
		while (1) {};
	}
}
