/**
 * @file
 *
 * @brief Jiffies implementaion.
 * @details jiffies are generating by mostly precision clock source with event device.
 *
 * @date 10.04.2012
 * @author Anton Bondarev
 */

#include <string.h>
#include <kernel/time/clock_source.h>
#include <kernel/time/time.h>

#define HZ      OPTION_GET(NUMBER, hz)
#define CS_NAME OPTION_GET(STRING, cs_name)

extern struct clock_source CLOCK_SOURCE_NAME(CS_NAME);

const struct clock_source *cs_jiffies = &CLOCK_SOURCE_NAME(CS_NAME);

clock_t clock_sys_ticks(void) {
	if (!cs_jiffies) {
		return 0;
	}
	return (clock_t)cs_jiffies->event_device->jiffies;
}

clock_t ns2jiffies(time64_t ns) {
	assert(cs_jiffies->event_device);
	return ns_to_clock(cs_jiffies->event_device->event_hz, ns);
}

clock_t ms2jiffies(time64_t ms) {
	return ns2jiffies(ms * 1000000);
}

time64_t jiffies2ms(clock_t jiff) {
	return clock_to_ns(cs_jiffies->event_device->event_hz, jiff) / 1000000;
}

uint32_t clock_freq(void) {
	return cs_jiffies->event_device->event_hz;
}

int jiffies_init(void) {
	extern int clock_tick_init(void);

	clock_tick_init();

	clock_source_register((struct clock_source *) cs_jiffies);

	clock_source_set_periodic((struct clock_source *) cs_jiffies, HZ);

	return 0;
}
