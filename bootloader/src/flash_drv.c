/**
 * @file    flash_drv.c
 * @brief   Flash 驱动实现
 * @details STM32F407 Flash 擦除和编程接口
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#include "flash_drv.h"

/* Flash 扇区地址定义 */
#define FLASH_SECTOR_0_ADDR     0x08000000  /* 16KB */
#define FLASH_SECTOR_1_ADDR     0x08004000  /* 16KB */
#define FLASH_SECTOR_2_ADDR     0x08008000  /* 16KB */
#define FLASH_SECTOR_3_ADDR     0x0800C000  /* 16KB */
#define FLASH_SECTOR_4_ADDR     0x08010000  /* 64KB */
#define FLASH_SECTOR_5_ADDR     0x08020000  /* 128KB */
#define FLASH_SECTOR_6_ADDR     0x08040000  /* 128KB */
#define FLASH_SECTOR_7_ADDR     0x08060000  /* 128KB */
#define FLASH_SECTOR_8_ADDR     0x08080000  /* 128KB */
#define FLASH_SECTOR_9_ADDR     0x080A0000  /* 128KB */
#define FLASH_SECTOR_10_ADDR    0x080C0000  /* 128KB */
#define FLASH_SECTOR_11_ADDR    0x080E0000  /* 128KB */
#define FLASH_END_ADDR          0x08100000

/*==============================================================================
 *                              寄存器定义
 *============================================================================*/

#define FLASH_BASE              0x40023C00

#define FLASH_ACR               (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_KEYR              (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_OPTKEYR           (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR                (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR                (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR             (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* FLASH_CR 位定义 */
#define FLASH_CR_PG             (1U << 0)
#define FLASH_CR_SER            (1U << 1)
#define FLASH_CR_MER            (1U << 2)
#define FLASH_CR_SNB_POS        3
#define FLASH_CR_SNB_MASK       (0xFU << FLASH_CR_SNB_POS)
#define FLASH_CR_PSIZE_POS      8
#define FLASH_CR_PSIZE_MASK     (0x3U << FLASH_CR_PSIZE_POS)
#define FLASH_CR_STRT           (1U << 16)
#define FLASH_CR_EOPIE          (1U << 24)
#define FLASH_CR_ERRIE          (1U << 25)
#define FLASH_CR_LOCK           (1U << 31)

/* FLASH_SR 位定义 */
#define FLASH_SR_EOP            (1U << 0)
#define FLASH_SR_SOP            (1U << 1)
#define FLASH_SR_WRPERR         (1U << 4)
#define FLASH_SR_PGAERR         (1U << 5)
#define FLASH_SR_PGPERR         (1U << 6)
#define FLASH_SR_PGSERR         (1U << 7)
#define FLASH_SR_BSY            (1U << 16)

/* Flash 密钥 */
#define FLASH_KEY1              0x45670123
#define FLASH_KEY2              0xCDEF89AB

/*==============================================================================
 *                              私有函数
 *============================================================================*/

/**
 * @brief  等待 Flash 操作完成
 * @return FLASH_OK - 成功
 */
static int FlashDrv_WaitForOperation(void)
{
    uint32_t timeout = 0xFFFFFF;
    
    while ((FLASH_SR & FLASH_SR_BSY) && timeout) {
        timeout--;
    }
    
    if (timeout == 0) {
        return FLASH_ERROR;
    }
    
    /* 检查错误 */
    if (FLASH_SR & (FLASH_SR_WRPERR | FLASH_SR_PGAERR | 
                    FLASH_SR_PGPERR | FLASH_SR_PGSERR)) {
        /* 清除错误标志 */
        FLASH_SR = FLASH_SR_WRPERR | FLASH_SR_PGAERR | 
                   FLASH_SR_PGPERR | FLASH_SR_PGSERR;
        return FLASH_ERROR;
    }
    
    /* 清除完成标志 */
    FLASH_SR = FLASH_SR_EOP;
    
    return FLASH_OK;
}

/*==============================================================================
 *                              公共函数
 *============================================================================*/

/**
 * @brief  初始化 Flash 驱动
 */
int FlashDrv_Init(void)
{
    /* 清除状态寄存器 */
    FLASH_SR = FLASH_SR_WRPERR | FLASH_SR_PGAERR | 
               FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_EOP;
    return FLASH_OK;
}

/**
 * @brief  解锁 Flash 编程
 */
int FlashDrv_Unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
    
    if (FLASH_CR & FLASH_CR_LOCK) {
        return FLASH_ERROR_LOCKED;
    }
    
    return FLASH_OK;
}

/**
 * @brief  锁定 Flash 编程
 */
void FlashDrv_Lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

/**
 * @brief  擦除指定扇区
 */
int FlashDrv_EraseSector(uint32_t sectorNum)
{
    int status;
    
    if (sectorNum > 11) {
        return FLASH_ERROR;
    }
    
    /* 等待前一次操作完成 */
    status = FlashDrv_WaitForOperation();
    if (status != FLASH_OK) {
        return status;
    }
    
    /* 配置擦除参数 */
    FLASH_CR &= ~FLASH_CR_SNB_MASK;
    FLASH_CR |= (sectorNum << FLASH_CR_SNB_POS) & FLASH_CR_SNB_MASK;
    
    /* 设置并行位数为 32 位 */
    FLASH_CR &= ~FLASH_CR_PSIZE_MASK;
    FLASH_CR |= (0x2U << FLASH_CR_PSIZE_POS);  /* 32-bit */
    
    /* 启动擦除 */
    FLASH_CR |= FLASH_CR_SER;
    FLASH_CR |= FLASH_CR_STRT;
    
    /* 等待完成 */
    status = FlashDrv_WaitForOperation();
    
    /* 清除擦除位 */
    FLASH_CR &= ~FLASH_CR_SER;
    
    return status;
}

/**
 * @brief  擦除应用程序区域
 */
int FlashDrv_EraseAppArea(void)
{
    int status;
    uint32_t sector;
    
    /* 擦除扇区 4-11 (App 区域) */
    for (sector = 4; sector <= 11; sector++) {
        status = FlashDrv_EraseSector(sector);
        if (status != FLASH_OK) {
            return status;
        }
    }
    
    return FLASH_OK;
}

/**
 * @brief  写入一个字 (32位)
 */
int FlashDrv_WriteWord(uint32_t addr, uint32_t data)
{
    int status;
    volatile uint32_t *flashAddr;
    
    /* 检查地址对齐 */
    if (addr & 0x3) {
        return FLASH_ERROR;
    }
    
    /* 检查地址范围 */
    if ((addr < FLASH_BASE_ADDR) || (addr >= FLASH_END_ADDR)) {
        return FLASH_ERROR;
    }
    
    /* 等待前一次操作完成 */
    status = FlashDrv_WaitForOperation();
    if (status != FLASH_OK) {
        return status;
    }
    
    /* 设置并行位数为 32 位 */
    FLASH_CR &= ~FLASH_CR_PSIZE_MASK;
    FLASH_CR |= (0x2U << FLASH_CR_PSIZE_POS);
    
    /* 启动编程 */
    FLASH_CR |= FLASH_CR_PG;
    
    /* 写入数据 */
    flashAddr = (volatile uint32_t *)addr;
    *flashAddr = data;
    
    /* 等待完成 */
    status = FlashDrv_WaitForOperation();
    
    /* 清除编程位 */
    FLASH_CR &= ~FLASH_CR_PG;
    
    return status;
}

/**
 * @brief  写入数据块
 */
int FlashDrv_Write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    int status;
    uint32_t word;
    
    /* 按字写入 */
    for (i = 0; i < (len & ~0x3); i += 4) {
        word = (uint32_t)data[i] |
               ((uint32_t)data[i + 1] << 8) |
               ((uint32_t)data[i + 2] << 16) |
               ((uint32_t)data[i + 3] << 24);
        
        status = FlashDrv_WriteWord(addr + i, word);
        if (status != FLASH_OK) {
            return status;
        }
    }
    
    /* 处理剩余字节 */
    if (len & 0x3) {
        word = 0xFFFFFFFF;
        for (i = len & ~0x3; i < len; i++) {
            word &= ~(0xFF << ((i & 0x3) * 8));
            word |= (uint32_t)data[i] << ((i & 0x3) * 8);
        }
        status = FlashDrv_WriteWord(addr + (len & ~0x3), word);
    }
    
    return status;
}

/**
 * @brief  读取数据
 */
void FlashDrv_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t i;
    volatile uint8_t *flashAddr = (volatile uint8_t *)addr;
    
    for (i = 0; i < len; i++) {
        data[i] = flashAddr[i];
    }
}

/**
 * @brief  获取扇区编号
 */
uint8_t FlashDrv_GetSector(uint32_t addr)
{
    if ((addr >= FLASH_SECTOR_0_ADDR) && (addr < FLASH_SECTOR_1_ADDR)) {
        return FLASH_SECTOR_0;
    } else if ((addr >= FLASH_SECTOR_1_ADDR) && (addr < FLASH_SECTOR_2_ADDR)) {
        return FLASH_SECTOR_1;
    } else if ((addr >= FLASH_SECTOR_2_ADDR) && (addr < FLASH_SECTOR_3_ADDR)) {
        return FLASH_SECTOR_2;
    } else if ((addr >= FLASH_SECTOR_3_ADDR) && (addr < FLASH_SECTOR_4_ADDR)) {
        return FLASH_SECTOR_3;
    } else if ((addr >= FLASH_SECTOR_4_ADDR) && (addr < FLASH_SECTOR_5_ADDR)) {
        return FLASH_SECTOR_4;
    } else if ((addr >= FLASH_SECTOR_5_ADDR) && (addr < FLASH_SECTOR_6_ADDR)) {
        return FLASH_SECTOR_5;
    } else if ((addr >= FLASH_SECTOR_6_ADDR) && (addr < FLASH_SECTOR_7_ADDR)) {
        return FLASH_SECTOR_6;
    } else if ((addr >= FLASH_SECTOR_7_ADDR) && (addr < FLASH_SECTOR_8_ADDR)) {
        return FLASH_SECTOR_7;
    } else if ((addr >= FLASH_SECTOR_8_ADDR) && (addr < FLASH_SECTOR_9_ADDR)) {
        return FLASH_SECTOR_8;
    } else if ((addr >= FLASH_SECTOR_9_ADDR) && (addr < FLASH_SECTOR_10_ADDR)) {
        return FLASH_SECTOR_9;
    } else if ((addr >= FLASH_SECTOR_10_ADDR) && (addr < FLASH_SECTOR_11_ADDR)) {
        return FLASH_SECTOR_10;
    } else if ((addr >= FLASH_SECTOR_11_ADDR) && (addr < FLASH_END_ADDR)) {
        return FLASH_SECTOR_11;
    }
    
    return 0xFF;
}
