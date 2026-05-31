/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 *
 * Minimal ARMv7-M startup for QEMU `mps2-an385` (Cortex-M3).
 * Provides the vector table, Reset_Handler, .data init, .bss zero, and
 * jumps to main(). All other vectors trap into a default handler.
 */
#include <stdint.h>

extern int main(void);

extern uint32_t _sidata, _sdata, _edata;   /* from linker script */
extern uint32_t _sbss,   _ebss;
extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);

void Reset_Handler(void)
{
    /* Copy .data initialisers from FLASH to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    /* Zero .bss */
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;

    (void)main();
    for (;;) { __asm__ volatile ("bkpt 0xAB"); }
}

void Default_Handler(void) { for (;;) { __asm__ volatile ("bkpt 0xAB"); } }

#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))
WEAK_ALIAS void NMI_Handler(void);
WEAK_ALIAS void HardFault_Handler(void);
WEAK_ALIAS void MemManage_Handler(void);
WEAK_ALIAS void BusFault_Handler(void);
WEAK_ALIAS void UsageFault_Handler(void);
WEAK_ALIAS void SVC_Handler(void);
WEAK_ALIAS void DebugMon_Handler(void);
WEAK_ALIAS void PendSV_Handler(void);
WEAK_ALIAS void SysTick_Handler(void);

__attribute__((section(".isr_vector"), used))
void (* const g_vectors[])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};

/* The newlib stubs that aren't provided by --specs=nano.specs. We don't use
 * malloc, file I/O, or the C library exit paths, so empty bodies suffice. */
void _exit(int code)            { (void)code; for (;;) { __asm__ volatile ("bkpt 0xAB"); } }
int  _close(int fd)             { (void)fd; return -1; }
int  _lseek(int fd, int o, int w){ (void)fd; (void)o; (void)w; return 0; }
int  _read(int fd, char *b, int n) { (void)fd; (void)b; (void)n; return 0; }
int  _write(int fd, const char *b, int n) { (void)fd; (void)b; return n; }
int  _fstat(int fd, void *st)   { (void)fd; (void)st; return 0; }
int  _isatty(int fd)            { (void)fd; return 0; }
int  _getpid(void)              { return 1; }
int  _kill(int pid, int sig)    { (void)pid; (void)sig; return -1; }
void *_sbrk(int incr)           { (void)incr; return (void*)-1; }
