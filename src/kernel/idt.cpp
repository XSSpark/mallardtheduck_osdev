#include "kernel.hpp"
#include "idt.hpp"
#include "locks.hpp"

/* Defines an IDT entry */
struct idt_entry
{
    uint16_t base_lo;
    uint16_t sel;        /* Our kernel segment goes here! */
    uint8_t always0;     /* This will ALWAYS be set to 0! */
    uint8_t flags;       /* Set using the above table! */
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct handler_context_t
{
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t interrupt_number;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t oresp;
    uint32_t ss;
} __attribute__((packed));

/* Declare an IDT of 256 entries. Although we will only use the
*  first 32 entries in this tutorial, the rest exists as a bit
*  of a trap. If any undefined IDT entry is hit, it normally
*  will cause an "Unhandled Interrupt" exception. Any descriptor
*  for which the 'presence' bit is cleared (0) will generate an
*  "Unhandled Interrupt" exception */
struct idt_entry idt[256];
struct idt_ptr idtp;

extern "C" void idt_flush();

/* Use this function to set an entry in the IDT. Alot simpler
*  than twiddling with the GDT ;) */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
	/* We'll leave you to try and code this function: take the
	*  argument 'base' and split it up into a high and low 16-bits,
	*  storing them in idt[num].base_hi and base_lo. The rest of the
	*  fields that you must set in idt[num] are fairly self-
	*  explanatory when it comes to setup */

	   idt[num].base_lo = base & 0xFFFF;
	   idt[num].base_hi = (base >> 16) & 0xFFFF;

	   idt[num].sel = sel;
	   idt[num].always0 = 0;
	   // We must uncomment the OR below when we get to using user-mode.
	   // It sets the interrupt gate's privilege level to 3.
	   idt[num].flags   = flags /* | 0x60 */;
}

/* Installs the IDT */
void IDT_init()
{
	dbgout("IDT init\n");
	/* Sets the special IDT pointer up, just like in 'gdt.c' */
	idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
	idtp.base = (uint32_t)&idt;

	/* Clear out the entire IDT, initializing it to zeros */
	memset(&idt, 0, sizeof(struct idt_entry) * 256);

	idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
	idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
	idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
	idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
	idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
	idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
	idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
	idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
	idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
	idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
	idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
	idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
	idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
	idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
	idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
	idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
	idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
	idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
	idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
	idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
	idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
	idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
	idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
	idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
	idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
	idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
	idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
	idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
	idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
	idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
	idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
	idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
	idt_set_gate(128, (uint32_t)isr128, 0x08, 0x8E);

	idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);

	/* Points the processor's internal register to the new IDT */
	idt_flush();
}

void out_int_info(const handler_context_t ctx){
	dbgpf("INTERRUPT %x ERRCODE: %x\n", ctx.interrupt_number, ctx.error_code);
	dbgpf("EAX: %x EBX: %x ECX: %x EDX: %x\n", ctx.eax, ctx.ebx, ctx.ecx, ctx.edx);
	dbgpf("EDI: %x ESI: %x EBP: %x ESP: %x\n", ctx.edi, ctx.esi, ctx.ebp, ctx.esp);
	dbgpf("EIP: %x CS: %x SS: %x\n", ctx.eip, ctx.cs, ctx.ss);
	dbgpf("EFLAGS: %x ORESP: %x\n", ctx.eflags, ctx.oresp);
}

extern "C" void isr_handler(handler_context_t ctx){
	dbgpf("\nInterrupt %i at %x!\n", ctx.interrupt_number, ctx.eip);
	
	if(ctx.interrupt_number==0x0D){
		out_int_info(ctx);
		panic("General Protection Fault.\n");
	}
	else if(ctx.interrupt_number==0x08){
		out_int_info(ctx);
		panic("Double fault.\n"); 
	}
	else if(ctx.interrupt_number==SYSCALL) printf("Syscall recieved.\n");
	out_int_info(ctx);
}

void irq_ack(size_t irq_no) {
	if (irq_no >= 12) {
		outb(0xA0, 0x20);
	}
	outb(0x20, 0x20);
}

extern lock ser_lock;

extern "C" void irq_handler(regs *r) {
	int irq=r->int_no-32;
	if(!try_take_lock(ser_lock)){
		irq_ack(irq);
		return;
	}
	release_lock(ser_lock);
	if(irq == 0){
		if(sch_isr(r)) irq_ack(irq);
	}
}
