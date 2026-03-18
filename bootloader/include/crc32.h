/**
 * @file    crc32.h
 * @brief   CRC32 校验头文件
 * @details IEEE 802.3 CRC-32 实现
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>

/*==============================================================================
 *                              宏定义
 *============================================================================*/

/* IEEE 802.3 CRC-32 参数 */
#define CRC32_INITIAL       0xFFFFFFFF
#define CRC32_FINAL_XOR     0xFFFFFFFF
#define CRC32_POLYNOMIAL    0x04C11DB7

/*==============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief  计算数据块的 CRC32
 * @param  data - 数据指针
 * @param  len  - 数据长度
 * @return CRC32 值
 */
uint32_t CRC32_Calculate(const uint8_t *data, uint32_t len);

/**
 * @brief  计算 Flash 区域的 CRC32
 * @param  addr - Flash 起始地址
 * @param  len  - 数据长度
 * @return CRC32 值
 */
uint32_t CRC32_CalculateFlash(uint32_t addr, uint32_t len);

/**
 * @brief  初始化 CRC32 查找表
 * @note   首次调用会自动初始化
 */
void CRC32_InitTable(void);

#endif /* CRC32_H */
