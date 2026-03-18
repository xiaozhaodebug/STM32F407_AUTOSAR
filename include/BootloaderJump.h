/**
 * @file    BootloaderJump.h
 * @brief   App 跳转到 Bootloader 功能
 * @details 实现通过 UDS 服务跳转到 Bootloader 进行 OTA 升级
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#ifndef BOOTLOADER_JUMP_H
#define BOOTLOADER_JUMP_H

#include <stdint.h>

/**
 * @brief  跳转到 Bootloader
 * @details 在 RAM 中设置标志，然后执行系统复位
 *          Bootloader 检测到标志后会停留在 Bootloader 模式
 */
void BootloaderJump_Trigger(void);

/**
 * @brief  检查是否刚从 Bootloader 启动
 * @return 1 表示刚从 Bootloader 启动，0 表示正常启动
 */
uint8_t BootloaderJump_CheckBootFromLoader(void);

/**
 * @brief  清除 Bootloader 跳转标志
 */
void BootloaderJump_ClearFlag(void);

#endif /* BOOTLOADER_JUMP_H */
