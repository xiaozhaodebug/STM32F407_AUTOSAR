/**
 * @file    boot.h
 * @brief   Bootloader 启动控制模块头文件
 * @details 实现应用程序有效性检查和跳转
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>

/*==============================================================================
 *                              宏定义
 *============================================================================*/

/* 应用程序起始地址 */
#define APP_START_ADDRESS       0x08010000
#define APP_VECTOR_TABLE_ADDR   (APP_START_ADDRESS)
#define APP_RESET_HANDLER_ADDR  (APP_START_ADDRESS + 4)

/* 应用程序结束地址 (1MB Flash) */
#define APP_END_ADDRESS         0x08100000

/* 应用程序有效性标志地址 (Flash 最后 16 字节) */
#define APP_VALID_MAGIC         0x5A5A
#define APP_VALID_FLAG_ADDR     0x080FFFF0

/* Flash 扇区定义 (STM32F407) */
#define FLASH_SECTOR_0_ADDR     0x08000000  /* 16KB - Bootloader */
#define FLASH_SECTOR_1_ADDR     0x08004000  /* 16KB - Bootloader */
#define FLASH_SECTOR_2_ADDR     0x08008000  /* 16KB - Bootloader */
#define FLASH_SECTOR_3_ADDR     0x0800C000  /* 16KB - Bootloader */
#define FLASH_SECTOR_4_ADDR     0x08010000  /* 64KB - App Start */
#define FLASH_SECTOR_5_ADDR     0x08020000  /* 128KB */
#define FLASH_SECTOR_6_ADDR     0x08040000  /* 128KB */
#define FLASH_SECTOR_7_ADDR     0x08060000  /* 128KB */
#define FLASH_SECTOR_8_ADDR     0x08080000  /* 128KB */
#define FLASH_SECTOR_9_ADDR     0x080A0000  /* 128KB */
#define FLASH_SECTOR_10_ADDR    0x080C0000  /* 128KB */
#define FLASH_SECTOR_11_ADDR    0x080E0000  /* 128KB */

/*==============================================================================
 *                              类型定义
 *============================================================================*/

typedef enum {
    BOOT_MODE_NORMAL,       /**< 正常启动到 App */
    BOOT_MODE_PROGRAMMING,  /**< 停留在 Bootloader */
    BOOT_MODE_UPDATING      /**< 正在升级 */
} Boot_ModeType;

typedef enum {
    BOOT_OK = 0,
    BOOT_ERR_INVALID_APP,
    BOOT_ERR_FLASH,
    BOOT_ERR_TIMEOUT
} Boot_StatusType;

/*==============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief  Bootloader 初始化
 */
void Boot_Init(void);

/**
 * @brief  检查应用程序有效性并启动
 * @return BOOT_OK - 成功跳转到 App
 *         BOOT_ERR_INVALID_APP - App 无效，停留在 Bootloader
 */
Boot_StatusType Boot_CheckAndStart(void);

/**
 * @brief  跳转到应用程序
 * @note   此函数不会返回
 */
void Boot_JumpToApp(void);

/**
 * @brief  停留在 Bootloader 模式
 */
void Boot_StayInBootloader(void);

/**
 * @brief  设置应用程序有效性标志
 * @param  valid - 0 无效, 1 有效
 */
void Boot_SetAppValid(uint8_t valid);

/**
 * @brief  检查应用程序是否有效
 * @return 0 - 无效, 1 - 有效
 */
uint8_t Boot_IsAppValid(void);

/**
 * @brief  获取启动模式
 */
Boot_ModeType Boot_GetMode(void);

/**
 * @brief  强制进入 Bootloader（用于按键触发）
 */
void Boot_ForceEnterBootloader(void);

#endif /* BOOT_H */
