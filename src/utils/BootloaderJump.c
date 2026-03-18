/**
 * @file    BootloaderJump.c
 * @brief   App 跳转到 Bootloader 功能实现
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#include "BootloaderJump.h"

/* RAM 标志地址 (在栈顶附近，不会被正常程序使用) */
#define BOOTLOADER_FLAG_ADDR    0x2001FFF0
#define BOOTLOADER_FLAG_VALUE   0x5A5A5A5A

/* 系统复位寄存器 */
#define SCB_AIRCR               (*(volatile uint32_t *)0xE000ED0C)
#define SCB_AIRCR_VECTKEY       (0x05FAU << 16)
#define SCB_AIRCR_SYSRESETREQ   (1U << 2)

/**
 * @brief  跳转到 Bootloader
 */
void BootloaderJump_Trigger(void)
{
    /* 在 RAM 中设置标志 */
    volatile uint32_t *flag = (volatile uint32_t *)BOOTLOADER_FLAG_ADDR;
    *flag = BOOTLOADER_FLAG_VALUE;
    
    /* 执行系统复位 */
    SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    
    /* 等待复位 */
    while (1) {
        __asm volatile ("nop");
    }
}

/**
 * @brief  检查是否刚从 Bootloader 启动
 */
uint8_t BootloaderJump_CheckBootFromLoader(void)
{
    volatile uint32_t *flag = (volatile uint32_t *)BOOTLOADER_FLAG_ADDR;
    return (*flag == BOOTLOADER_FLAG_VALUE) ? 1 : 0;
}

/**
 * @brief  清除 Bootloader 跳转标志
 */
void BootloaderJump_ClearFlag(void)
{
    volatile uint32_t *flag = (volatile uint32_t *)BOOTLOADER_FLAG_ADDR;
    *flag = 0;
}
