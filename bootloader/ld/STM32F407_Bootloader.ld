/*
 * STM32F407 Bootloader Linker Script
 * Target: STM32F407ZGT6
 * Bootloader: 0x08000000 - 0x0800FFFF (64KB)
 * App Start: 0x08010000
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

__min_heap_size = 0x200;
__min_stack_size = 0x400;

MEMORY
{
    /* Bootloader Flash - 前64KB (0x08000000 - 0x0800FFFF) */
    BOOT_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    
    /* RAM */
    RAM (rwx)       : ORIGIN = 0x20000000, LENGTH = 128K
    CCM (rwx)       : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    /* 中断向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > BOOT_FLASH

    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > BOOT_FLASH

    /* 只读数据 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > BOOT_FLASH

    /* ARM exception unwinding section */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > BOOT_FLASH

    /* ARM index table */
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > BOOT_FLASH

    /* Pre-initialized data */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> BOOT_FLASH

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* User heap section */
    ._user_heap :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + __min_heap_size;
        . = ALIGN(8);
    } > RAM

    /* Stack at end of RAM */
    __stack_bottom = _estack - __min_stack_size;

    /* Remove debug information from the executable */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
