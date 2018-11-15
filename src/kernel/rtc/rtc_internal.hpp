#ifndef _RTC_INTERNAL_HPP
#define _RTC_INTERNAL_HPP

#include "../kernel.hpp"

//ISO-8061-ish format. Using 'T' as a delimeter instead of space (as ISO dictates) is bad for reaability, so is not used.
#define FORMAT "%02i-%02i-%02i %02i:%02i:%02i"

extern uint16_t extension_id;

struct datetime{
	int day, month, year;
	int hour, minute, second;
};

uint64_t get_msecs();
datetime current_datetime();

void init_timer();
void create_timer(isr_regs *regs);
void reset_timer(isr_regs *regs);

datetime epoch2datetime(uint64_t ep);
uint64_t datetime2epoch(const datetime &dt);

bool timer_ready_blockcheck(void*);

#endif