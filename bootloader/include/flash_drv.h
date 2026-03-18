/**
 * @file    flash_drv.h
 * @brief   Flash 驱动头文件
 * @details STM32F407 Flash 擦除和编程接口
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#ifndef FLASH_DRV_H
#define FLASH_DRV_H

#include <stdint.h>

/*==============================================================================
 *                              宏定义
 *============================================================================*/

/* Flash 编程相关 */
#define FLASH_WORD_SIZE         4       /**< 字大小 (32位) */
#define FLASH_DOUBLE_WORD_SIZE  8       /**< 双字大小 (64位) */

/* Flash 扇区编号 */
#define FLASH_SECTOR_0          0       /**< 16KB */
#define FLASH_SECTOR_1          1       /**< 16KB */
#define FLASH_SECTOR_2          2       /**< 16KB */
#define FLASH_SECTOR_3          3       /**< 16KB */
#define FLASH_SECTOR_4          4       /**< 64KB */
#define FLASH_SECTOR_5          5       /**< 128KB */
#define FLASH_SECTOR_6          6       /**< 128KB */
#define FLASH_SECTOR_7          7       /**< 128KB */
#define FLASH_SECTOR_8          8       /**< 128KB */
#define FLASH_SECTOR_9          9       /**< 128KB */
#define FLASH_SECTOR_10         10      /**< 128KB */
#define FLASH_SECTOR_11         11      /**< 128KB */

/* Flash 地址范围 */
#define FLASH_BASE_ADDR         0x08000000
#define FLASH_END_ADDR          0x08100000
#define FLASH_SIZE              (1024 * 1024)  /* 1MB */

/* 返回值 */
#define FLASH_OK                0
#define FLASH_ERROR             1
#define FLASH_ERROR_LOCKED      2
#define FLASH_ERROR_PROGRAM     3
#define FLASH_ERROR_ERASE       4

/*==============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief  初始化 Flash 驱动
 * @return FLASH_OK - 成功
 */
int FlashDrv_Init(void);

/**
 * @brief  擦除指定扇区
 * @param  sectorNum - 扇区编号 (0-11)
 * @return FLASH_OK - 成功
 */
int FlashDrv_EraseSector(uint32_t sectorNum);

/**
 * @brief  擦除应用程序区域
 * @details 擦除从扇区 4 开始的所有扇区
 * @return FLASH_OK - 成功
 */
int FlashDrv_EraseAppArea(void);

/**
 * @brief  写入一个字 (32位)
 * @param  addr - 目标地址 (必须对齐到 4 字节)
 * @param  data - 数据
 * @return FLASH_OK - 成功
 */
int FlashDrv_WriteWord(uint32_t addr, uint32_t data);

/**
 * @brief  写入数据块
 * @param  addr - 目标地址
 * @param  data - 数据指针
 * @param  len  - 数据长度 (字节)
 * @return FLASH_OK - 成功
 */
int FlashDrv_Write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * @brief  读取数据
 * @param  addr - 源地址
 * @param  data - 数据缓冲区
 * @param  len  - 数据长度
 */
void FlashDrv_Read(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief  解锁 Flash 编程
 * @return FLASH_OK - 成功
 */
int FlashDrv_Unlock(void);

/**
 * @brief  锁定 Flash 编程
 */
void FlashDrv_Lock(void);

/**
 * @brief  获取扇区编号
 * @param  addr - Flash 地址
 * @return 扇区编号 (0-11), 无效地址返回 0xFF
 */
uint8_t FlashDrv_GetSector(uint32_t addr);

#endif /* FLASH_DRV_H */
