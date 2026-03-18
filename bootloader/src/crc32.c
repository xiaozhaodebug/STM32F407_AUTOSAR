/**
 * @file    crc32.c
 * @brief   CRC32 校验实现
 * @details IEEE 802.3 CRC-32
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#include "crc32.h"

/*==============================================================================
 *                              私有变量
 *============================================================================*/

static uint32_t crc32_table[256];
static uint8_t crc32_initialized = 0;

/*==============================================================================
 *                              公共函数
 *============================================================================*/

/**
 * @brief  初始化 CRC32 查找表
 */
void CRC32_InitTable(void)
{
    uint32_t i, j, crc;
    
    for (i = 0; i < 256; i++) {
        crc = i << 24;
        for (j = 0; j < 8; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
        crc32_table[i] = crc;
    }
    
    crc32_initialized = 1;
}

/**
 * @brief  计算数据块的 CRC32
 */
uint32_t CRC32_Calculate(const uint8_t *data, uint32_t len)
{
    uint32_t crc = CRC32_INITIAL;
    uint32_t i;
    
    if (!crc32_initialized) {
        CRC32_InitTable();
    }
    
    for (i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ data[i]) & 0xFF];
    }
    
    return crc ^ CRC32_FINAL_XOR;
}

/**
 * @brief  计算 Flash 区域的 CRC32
 */
uint32_t CRC32_CalculateFlash(uint32_t addr, uint32_t len)
{
    uint32_t crc = CRC32_INITIAL;
    uint32_t i;
    volatile uint8_t *p = (volatile uint8_t *)addr;
    
    if (!crc32_initialized) {
        CRC32_InitTable();
    }
    
    for (i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ p[i]) & 0xFF];
    }
    
    return crc ^ CRC32_FINAL_XOR;
}
