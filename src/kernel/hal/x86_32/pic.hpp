#ifndef _PIC_HPP
#define _PIC_HPP

void PIC_init();
void IRQ_set_mask(unsigned char IRQline);
void IRQ_clear_mask(unsigned char IRQline);
void PIC_sendEOI(unsigned char irq);
uint16_t pic_get_isr();

#endif
