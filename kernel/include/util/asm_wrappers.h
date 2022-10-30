#ifndef __K_UTIL_ASM_WRAPPERS_H
#define __K_UTIL_ASM_WRAPPERS_H

#include <stdint.h>

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void halt();
void cli();
void sti();

#endif