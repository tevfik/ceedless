/* SPDX-License-Identifier: MIT
 * QEMU mps2-an385 (Cortex-M3) memory map. 4 MB of FLASH at 0x0, 4 MB of
 * RAM at 0x20000000. The vectors live at the start of FLASH; the stack
 * starts at the top of RAM and grows down.
 */
ENTRY(Reset_Handler)

MEMORY {
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .isr_vector : {
        KEEP(*(.isr_vector))
    } > FLASH

    .text : {
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    /* .data is loaded from FLASH, copied to RAM by Reset_Handler */
    _sidata = LOADADDR(.data);
    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    /DISCARD/ : { *(.ARM.attributes) *(.comment) }
}
