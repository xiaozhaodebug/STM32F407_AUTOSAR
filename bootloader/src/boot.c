/**
 * @file    boot.c
 * @brief   Bootloader 启动控制模块
 * @details 实现应用程序有效性检查和跳转
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#include "boot.h"
#include "flash_drv.h"
#include <string.h>

/*==============================================================================
 *                              私有变量
 *============================================================================*/

static Boot_ModeType gBootMode = BOOT_MODE_NORMAL;

/*==============================================================================
 *                              私有函数
 *============================================================================*/

/**
 * @brief  关闭中断
 */
static void Boot_DisableInterrupts(void)
{
    __asm volatile ("cpsid i");
}

/**
 * @brief  设置主栈指针
 */
static void Boot_SetMSP(uint32_t topOfStack)
{
    __asm volatile ("msr msp, %0" : : "r" (topOfStack));
}

/*==============================================================================
 *                              公共函数
 *============================================================================*/

/**
 * @brief  Bootloader 初始化
 */
void Boot_Init(void)
{
    gBootMode = BOOT_MODE_NORMAL;
}

/**
 * @brief  检查应用程序是否有效
 * @return 0 - 无效, 1 - 有效
 */
uint8_t Boot_IsAppValid(void)
{
    /* 检查有效性标志 */
    volatile uint16_t *validFlag = (volatile uint16_t *)APP_VALID_FLAG_ADDR;
    if (*validFlag != APP_VALID_MAGIC) {
        return 0;
    }
    
    /* 检查向量表有效性 - 栈顶地址应该在 RAM 范围内 */
    uint32_t spValue = *((volatile uint32_t *)APP_VECTOR_TABLE_ADDR);
    if ((spValue < 0x20000000) || (spValue > 0x20030000)) {
        return 0;
    }
    
    /* 检查 Reset Handler 地址有效性 - 应该在 App 范围内 */
    uint32_t pcValue = *((volatile uint32_t *)APP_RESET_HANDLER_ADDR);
    if ((pcValue < APP_START_ADDRESS) || (pcValue >= APP_END_ADDRESS)) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief  跳转到应用程序
 * @note   此函数不会返回
 */
void Boot_JumpToApp(void)
{
    typedef void (*App_ResetHandler)(void);
    
    App_ResetHandler appResetHandler;
    uint32_t spValue;
    
    /* 获取栈顶地址 */
    spValue = *((volatile uint32_t *)APP_VECTOR_TABLE_ADDR);
    
    /* 获取复位处理函数地址 */
    appResetHandler = (App_ResetHandler)(*((volatile uint32_t *)APP_RESET_HANDLER_ADDR));
    
    /* 关闭中断 */
    Boot_DisableInterrupts();
    
    /* 设置主栈指针 */
    Boot_SetMSP(spValue);
    
    /* 跳转到应用程序 */
    appResetHandler();
    
    /* 不应该到达这里 */
    while (1);
}

/**
 * @brief  停留在 Bootloader 模式
 */
void Boot_StayInBootloader(void)
{
    gBootMode = BOOT_MODE_PROGRAMMING;
}

/**
 * @brief  检查应用程序有效性并启动
 * @return BOOT_OK - 成功跳转到 App
 *         BOOT_ERR_INVALID_APP - App 无效，停留在 Bootloader
 */
Boot_StatusType Boot_CheckAndStart(void)
{
    if (Boot_IsAppValid()) {
        /* App 有效，跳转 */
        Boot_JumpToApp();
        /* 不会返回 */
        return BOOT_OK;
    } else {
        /* App 无效，停留在 Bootloader */
        Boot_StayInBootloader();
        return BOOT_ERR_INVALID_APP;
    }
}

/**
 * @brief  设置应用程序有效性标志
 * @param  valid - 0 无效, 1 有效
 */
void Boot_SetAppValid(uint8_t valid)
{
    /* 使用简单的 Flash 操作 */
    volatile uint32_t *flashKeyReg = (volatile uint32_t *)0x40023C04;
    volatile uint32_t *flashCtrlReg = (volatile uint32_t *)0x40023C10;
    volatile uint32_t *flashStatusReg = (volatile uint32_t *)0x40023C0C;
    volatile uint32_t *flagAddr = (volatile uint32_t *)APP_VALID_FLAG_ADDR;
    uint32_t timeout;
    
    (void)valid; /* 防止未使用警告 */
    
    /* 解锁 Flash */
    *flashKeyReg = 0x45670123;
    *flashKeyReg = 0xCDEF89AB;
    
    /* 擦除扇区 11 */
    *flashCtrlReg &= ~(0xFU << 3);  /* 清除 SNB */
    *flashCtrlReg |= (11U << 3);    /* 设置扇区号 */
    *flashCtrlReg |= (1U << 1);     /* SER = 1 */
    *flashCtrlReg |= (1U << 16);    /* STRT = 1 */
    
    /* 等待擦除完成 */
    timeout = 0xFFFFFF;
    while ((*flashStatusReg & (1U << 16)) && timeout) {
        timeout--;
    }
    
    *flashCtrlReg &= ~(1U << 1);    /* SER = 0 */
    
    /* 写入标志 (0x5A5A 作为 32-bit 值) */
    *flashCtrlReg |= (1U << 0);     /* PG = 1 */
    *flagAddr = 0x00005A5A;
    
    /* 等待写入完成 */
    timeout = 0xFFFFFF;
    while ((*flashStatusReg & (1U << 16)) && timeout) {
        timeout--;
    }
    
    *flashCtrlReg &= ~(1U << 0);    /* PG = 0 */
    *flashCtrlReg |= (1U << 31);    /* LOCK = 1 */
}

/**
 * @brief  获取启动模式
 */
Boot_ModeType Boot_GetMode(void)
{
    return gBootMode;
}

/**
 * @brief  强制进入 Bootloader
 */
void Boot_ForceEnterBootloader(void)
{
    gBootMode = BOOT_MODE_PROGRAMMING;
}
