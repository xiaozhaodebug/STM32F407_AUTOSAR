/**
 * @file    main_bootloader.c
 * @brief   Bootloader 主程序
 * @details STM32F407 Bootloader 入口，支持 UDS OTA 升级
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#include <stdint.h>
#include <string.h>
#include "boot.h"
#include "flash_drv.h"
#include "uds_bootloader.h"
#include "crc32.h"

/*==============================================================================
 *                              寄存器定义
 *============================================================================*/

#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRH      (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

#define GPIOE_BASE      0x40021000
#define GPIOE_MODER     (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_ODR       (*(volatile uint32_t *)(GPIOE_BASE + 0x14))
#define GPIOE_BSRR      (*(volatile uint32_t *)(GPIOE_BASE + 0x18))

#define CAN1_BASE       0x40006400
#define CAN1_MCR        (*(volatile uint32_t *)(CAN1_BASE + 0x000))
#define CAN1_MSR        (*(volatile uint32_t *)(CAN1_BASE + 0x004))
#define CAN1_TSR        (*(volatile uint32_t *)(CAN1_BASE + 0x008))
#define CAN1_RF0R       (*(volatile uint32_t *)(CAN1_BASE + 0x00C))
#define CAN1_IER        (*(volatile uint32_t *)(CAN1_BASE + 0x014))
#define CAN1_BTR        (*(volatile uint32_t *)(CAN1_BASE + 0x01C))
#define CAN1_TI0R       (*(volatile uint32_t *)(CAN1_BASE + 0x180))
#define CAN1_TDT0R      (*(volatile uint32_t *)(CAN1_BASE + 0x184))
#define CAN1_TDL0R      (*(volatile uint32_t *)(CAN1_BASE + 0x188))
#define CAN1_TDH0R      (*(volatile uint32_t *)(CAN1_BASE + 0x18C))
#define CAN1_RI0R       (*(volatile uint32_t *)(CAN1_BASE + 0x1B0))
#define CAN1_RDT0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1B4))
#define CAN1_RDL0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1B8))
#define CAN1_RDH0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1BC))
#define CAN1_FMR        (*(volatile uint32_t *)(CAN1_BASE + 0x200))
#define CAN1_FM1R       (*(volatile uint32_t *)(CAN1_BASE + 0x204))
#define CAN1_FS1R       (*(volatile uint32_t *)(CAN1_BASE + 0x20C))
#define CAN1_FFA1R      (*(volatile uint32_t *)(CAN1_BASE + 0x214))
#define CAN1_FA1R       (*(volatile uint32_t *)(CAN1_BASE + 0x21C))
#define CAN1_F0R1       (*(volatile uint32_t *)(CAN1_BASE + 0x240))
#define CAN1_F0R2       (*(volatile uint32_t *)(CAN1_BASE + 0x244))

#define USART1_BASE     0x40011000
#define USART1_SR       (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR       (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define SYSTICK_BASE    0xE000E010
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))

#define NVIC_ISER0      (*(volatile uint32_t *)0xE000E100)

/* UDS CAN ID 定义 */
#define UDS_PHYS_REQUEST_ID     0x735   /* 物理请求 ID */
#define UDS_FUNC_REQUEST_ID     0x7DF   /* 功能请求 ID */
#define UDS_RESPONSE_ID         0x73D   /* 响应 ID */

/*==============================================================================
 *                              调试串口
 *============================================================================*/

static void Debug_Init(void)
{
    /* 使能 GPIOA 和 USART1 时钟 */
    RCC_AHB1ENR |= (1U << 0);
    RCC_APB1ENR |= (1U << 4);  /* USART1 在 APB2，这里是简化的 */
    
    /* 配置 PA9 (TX) 为复用功能 */
    GPIOA_MODER &= ~(3U << 18);
    GPIOA_MODER |= (2U << 18);
    GPIOA_AFRH &= ~(0xFU << 4);
    GPIOA_AFRH |= (7U << 4);
    
    /* 配置波特率 115200 (84MHz / 115200 = 729.17) */
    USART1_BRR = 0x02D9;
    
    /* 使能发送 */
    USART1_CR1 = (1U << 3) | (1U << 13);
}

static void Debug_PutChar(char c)
{
    while (!(USART1_SR & (1U << 7))) {
        /* 等待发送缓冲区空 */
    }
    USART1_DR = c;
}

static void Debug_Print(const char *str)
{
    while (*str) {
        Debug_PutChar(*str++);
    }
}

static void Debug_PrintHex(uint8_t val)
{
    const char hex[] = "0123456789ABCDEF";
    Debug_PutChar(hex[val >> 4]);
    Debug_PutChar(hex[val & 0x0F]);
}

/*==============================================================================
 *                              LED 指示
 *============================================================================*/

static void LED_Init(void)
{
    /* 使能 GPIOE 时钟 */
    RCC_AHB1ENR |= (1U << 4);
    
    /* 配置 PE3, PE4 为输出 */
    GPIOE_MODER &= ~((3U << 6) | (3U << 8));
    GPIOE_MODER |= ((1U << 6) | (1U << 8));
    
    /* 初始熄灭 (高电平) */
    GPIOE_BSRR = ((1U << 3) | (1U << 4));
}

static void LED_Set(uint8_t led, uint8_t on)
{
    if (led == 0) {
        if (on) {
            GPIOE_BSRR = (1U << (3 + 16));  /* PE3 点亮 */
        } else {
            GPIOE_BSRR = (1U << 3);          /* PE3 熄灭 */
        }
    } else if (led == 1) {
        if (on) {
            GPIOE_BSRR = (1U << (4 + 16));  /* PE4 点亮 */
        } else {
            GPIOE_BSRR = (1U << 4);          /* PE4 熄灭 */
        }
    }
}

static void LED_Toggle(uint8_t led)
{
    static uint8_t state[2] = {0, 0};
    state[led] = !state[led];
    LED_Set(led, state[led]);
}

/*==============================================================================
 *                              CAN 驱动
 *============================================================================*/

static volatile uint8_t gCanRxFlag = 0;
static volatile uint32_t gCanRxId = 0;
static volatile uint8_t gCanRxData[8];
static volatile uint8_t gCanRxDlc = 0;

static void CAN_Init(void)
{
    uint32_t timeout;
    
    /* 使能 GPIOA 时钟 */
    RCC_AHB1ENR |= (1U << 0);
    
    /* 配置 PA11 (CAN1_RX) 和 PA12 (CAN1_TX) */
    GPIOA_MODER &= ~(3U << 22);
    GPIOA_MODER |= (2U << 22);
    GPIOA_MODER &= ~(3U << 24);
    GPIOA_MODER |= (2U << 24);
    
    /* 设置复用功能 AF9 (CAN1) */
    GPIOA_AFRH &= ~(0xFU << 12);
    GPIOA_AFRH |= (9U << 12);
    GPIOA_AFRH &= ~(0xFU << 16);
    GPIOA_AFRH |= (9U << 16);
    
    /* 使能 CAN1 时钟 */
    RCC_APB1ENR |= (1U << 25);
    
    /* 退出睡眠模式 */
    CAN1_MCR &= ~(1U << 1);
    timeout = 10000;
    while ((CAN1_MSR & (1U << 1)) && timeout > 0) {
        timeout--;
    }
    
    /* 请求初始化模式 */
    CAN1_MCR |= (1U << 0);
    timeout = 10000;
    while ((!(CAN1_MSR & (1U << 0))) && timeout > 0) {
        timeout--;
    }
    
    /* 配置波特率 500Kbps (APB1 = 42MHz) */
    CAN1_BTR = 0;
    CAN1_BTR |= (5U << 0);      /* BRP = 6-1 = 5 */
    CAN1_BTR |= (10U << 16);    /* TS1 = 11-1 = 10 */
    CAN1_BTR |= (1U << 20);     /* TS2 = 2-1 = 1 */
    
    /* 请求正常模式 */
    CAN1_MCR &= ~(1U << 0);
    timeout = 10000;
    while ((CAN1_MSR & (1U << 0)) && timeout > 0) {
        timeout--;
    }
    
    /* 配置过滤器 - 接收所有消息 */
    CAN1_FMR |= (1U << 0);
    CAN1_FA1R &= ~(1U << 0);
    CAN1_FS1R |= (1U << 0);
    CAN1_FM1R &= ~(1U << 0);
    CAN1_FFA1R &= ~(1U << 0);
    CAN1_F0R1 = 0x00000000;
    CAN1_F0R2 = 0x00000000;
    CAN1_FA1R |= (1U << 0);
    CAN1_FMR &= ~(1U << 0);
    
    /* 使能接收中断 */
    CAN1_IER |= (1U << 1);
    NVIC_ISER0 |= (1U << 20);
}

static void CAN_Send(uint32_t id, const uint8_t *data, uint8_t dlc)
{
    uint32_t timeout = 10000;
    uint8_t i;
    
    /* 等待发送邮箱空 */
    while (!(CAN1_TSR & (1U << 26)) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        return;  /* 发送超时 */
    }
    
    /* 设置 ID */
    CAN1_TI0R = (id << 21);  /* 标准 ID */
    
    /* 设置数据长度 */
    CAN1_TDT0R = dlc & 0x0F;
    
    /* 设置数据 */
    CAN1_TDL0R = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    CAN1_TDH0R = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
    
    /* 请求发送 */
    CAN1_TI0R |= (1U << 0);
}

static uint8_t CAN_Receive(uint32_t *id, uint8_t *data, uint8_t *dlc)
{
    if (!gCanRxFlag) {
        return 0;  /* 没有新消息 */
    }
    
    *id = gCanRxId;
    *dlc = gCanRxDlc;
    memcpy(data, (const void *)gCanRxData, gCanRxDlc);
    
    gCanRxFlag = 0;  /* 清除标志 */
    return 1;
}

/* CAN 接收中断处理 */
void CAN1_RX0_IRQHandler(void)
{
    /* 读取消息 */
    gCanRxId = (CAN1_RI0R >> 21) & 0x7FF;
    gCanRxDlc = CAN1_RDT0R & 0x0F;
    
    /* 读取数据 */
    uint32_t low = CAN1_RDL0R;
    uint32_t high = CAN1_RDH0R;
    
    gCanRxData[0] = low & 0xFF;
    gCanRxData[1] = (low >> 8) & 0xFF;
    gCanRxData[2] = (low >> 16) & 0xFF;
    gCanRxData[3] = (low >> 24) & 0xFF;
    gCanRxData[4] = high & 0xFF;
    gCanRxData[5] = (high >> 8) & 0xFF;
    gCanRxData[6] = (high >> 16) & 0xFF;
    gCanRxData[7] = (high >> 24) & 0xFF;
    
    gCanRxFlag = 1;
    
    /* 释放 FIFO */
    CAN1_RF0R |= (1U << 5);
}

/*==============================================================================
 *                              UDS 处理
 *============================================================================*/

static uint8_t gUdsRxBuffer[256];
static uint16_t gUdsRxLen = 0;
static uint8_t gUdsTxBuffer[256];
static uint16_t gUdsTxLen = 0;
static uint8_t gUdsSessionActive = 0;

/**
 * @brief  解析 ISO-TP 单帧
 */
static uint8_t ParseSingleFrame(const uint8_t *data, uint8_t dlc, uint8_t *outData, uint16_t *outLen)
{
    if (dlc < 1) {
        return 0;
    }
    
    uint8_t len = data[0] & 0x0F;
    if (len > 7 || len + 1 > dlc) {
        return 0;
    }
    
    memcpy(outData, &data[1], len);
    *outLen = len;
    return 1;
}

/**
 * @brief  构建 ISO-TP 单帧
 */
static void BuildSingleFrame(const uint8_t *data, uint8_t len, uint8_t *outData, uint8_t *outDlc)
{
    outData[0] = len;
    memcpy(&outData[1], data, len);
    
    /* 填充剩余字节 */
    for (uint8_t i = len + 1; i < 8; i++) {
        outData[i] = 0x00;
    }
    
    *outDlc = 8;
}

/**
 * @brief  发送 UDS 否定响应
 */
static void SendNegativeResponse(uint8_t sid, uint8_t nrc)
{
    uint8_t response[8];
    uint8_t dlc;
    
    response[0] = 0x7F;  /* 否定响应 SID */
    response[1] = sid;
    response[2] = nrc;
    
    BuildSingleFrame(response, 3, gUdsTxBuffer, &dlc);
    CAN_Send(UDS_RESPONSE_ID, gUdsTxBuffer, dlc);
}

/**
 * @brief  发送 UDS 肯定响应
 */
static void SendPositiveResponse(const uint8_t *data, uint8_t len)
{
    uint8_t dlc;
    BuildSingleFrame(data, len, gUdsTxBuffer, &dlc);
    CAN_Send(UDS_RESPONSE_ID, gUdsTxBuffer, dlc);
}

/**
 * @brief  处理 UDS 请求
 */
static void ProcessUdsRequest(const uint8_t *data, uint16_t len)
{
    if (len < 1) {
        return;
    }
    
    uint8_t sid = data[0];
    uint8_t response[256];
    uint16_t responseLen = 0;
    Std_ReturnType result;
    
    LED_Toggle(0);  /* LED 闪烁表示收到请求 */
    
    switch (sid) {
        case 0x10:  /* DiagnosticSessionControl */
            if (len >= 2) {
                uint8_t sessionType = data[1];
                response[0] = 0x50;  /* 肯定响应 SID */
                response[1] = sessionType;
                response[2] = 0x00;  /* P2 高字节 */
                response[3] = 0x32;  /* P2 低字节 (50ms) */
                response[4] = 0x01;  /* P2* 高字节 */
                response[5] = 0xF4;  /* P2* 低字节 (500ms) */
                SendPositiveResponse(response, 6);
                
                if (sessionType == 0x02) {
                    gUdsSessionActive = 1;  /* 编程会话 */
                }
            } else {
                SendNegativeResponse(sid, 0x13);  /* 消息长度错误 */
            }
            break;
            
        case 0x11:  /* ECUReset */
            if (len >= 2) {
                uint8_t resetType = data[1];
                response[0] = 0x51;  /* 肯定响应 SID */
                response[1] = resetType;
                SendPositiveResponse(response, 2);
                
                /* 延时后复位 */
                for (volatile uint32_t i = 0; i < 1000000; i++);
                
                /* 执行系统复位 */
                *(volatile uint32_t *)0xE000ED0C = 0x05FA0004;
            }
            break;
            
        case 0x22:  /* ReadDataByIdentifier */
            if (len >= 3) {
                uint16_t did = (data[1] << 8) | data[2];
                response[0] = 0x62;  /* 肯定响应 SID */
                response[1] = data[1];
                response[2] = data[2];
                
                if (did == 0xF180) {  /* Bootloader 版本 */
                    response[3] = 0x01;  /* 主版本 */
                    response[4] = 0x00;  /* 次版本 */
                    SendPositiveResponse(response, 5);
                } else if (did == 0xF197) {  /* 系统名称 */
                    const char *name = "STM32F407";
                    memcpy(&response[3], name, 9);
                    SendPositiveResponse(response, 12);
                } else {
                    SendNegativeResponse(sid, 0x31);  /* 请求超出范围 */
                }
            }
            break;
            
        case 0x31:  /* RoutineControl */
            if (len >= 4) {
                uint8_t routineType = data[1];
                uint16_t routineId = (data[2] << 8) | data[3];
                
                if (routineId == ROUTINE_ERASE_MEMORY) {
                    result = UdsService_RoutineEraseMemory(data, len, response, &responseLen);
                    if (result == E_OK) {
                        SendPositiveResponse(response, responseLen);
                    } else {
                        SendNegativeResponse(sid, 0x22);
                    }
                } else if (routineId == ROUTINE_CHECK_INTEGRITY) {
                    result = UdsService_RoutineCheckIntegrity(data, len, response, &responseLen);
                    if (result == E_OK) {
                        SendPositiveResponse(response, responseLen);
                    } else {
                        SendNegativeResponse(sid, 0x22);
                    }
                } else {
                    SendNegativeResponse(sid, 0x31);
                }
            }
            break;
            
        case 0x34:  /* RequestDownload */
            if (gUdsSessionActive) {
                result = UdsService_RequestDownload(data, len, response, &responseLen);
                if (result == E_OK) {
                    SendPositiveResponse(response, responseLen);
                } else {
                    SendNegativeResponse(sid, 0x31);
                }
            } else {
                SendNegativeResponse(sid, 0x7F);  /* 服务在当前会话不支持 */
            }
            break;
            
        case 0x36:  /* TransferData */
            if (gUdsSessionActive) {
                result = UdsService_TransferData(data, len, response, &responseLen);
                if (result == E_OK) {
                    SendPositiveResponse(response, responseLen);
                } else {
                    SendNegativeResponse(sid, 0x71);  /* 传输数据失败 */
                }
            } else {
                SendNegativeResponse(sid, 0x7F);
            }
            break;
            
        case 0x37:  /* RequestTransferExit */
            if (gUdsSessionActive) {
                result = UdsService_RequestTransferExit(data, len, response, &responseLen);
                if (result == E_OK) {
                    SendPositiveResponse(response, responseLen);
                    UdsBootloader_FinishProgramming();  /* 设置 App 有效 */
                } else {
                    SendNegativeResponse(sid, 0x72);
                }
            } else {
                SendNegativeResponse(sid, 0x7F);
            }
            break;
            
        case 0x3E:  /* TesterPresent */
            response[0] = 0x7E;  /* 肯定响应 SID */
            if (len >= 2 && data[1] == 0x80) {
                /* 抑制响应位 */
            } else {
                SendPositiveResponse(response, 1);
            }
            break;
            
        default:
            SendNegativeResponse(sid, 0x11);  /* 服务不支持 */
            break;
    }
}

/*==============================================================================
 *                              主函数
 *============================================================================*/

int main(void)
{
    uint32_t rxId;
    uint8_t rxData[8];
    uint8_t rxDlc;
    
    /* 初始化 */
    Debug_Init();
    LED_Init();
    FlashDrv_Init();
    Boot_Init();
    
    Debug_Print("\r\n========================================\r\n");
    Debug_Print("STM32F407 Bootloader v1.0\r\n");
    Debug_Print("UDS OTA Support Enabled\r\n");
    Debug_Print("========================================\r\n");
    
    /* 检查是否强制进入 Bootloader (例如检查特定 RAM 标志) */
    volatile uint32_t *bootFlag = (volatile uint32_t *)0x2001FFF0;
    if (*bootFlag == 0x5A5A5A5A) {
        Debug_Print("[BOOT] Force bootloader mode (RAM flag)\r\n");
        *bootFlag = 0;  /* 清除标志 */
        LED_Set(0, 1);  /* LED0 点亮 */
    } else {
        /* 尝试启动 App */
        Debug_Print("[BOOT] Checking App validity...\r\n");
        if (Boot_IsAppValid()) {
            Debug_Print("[BOOT] App valid, jumping...\r\n");
            Boot_JumpToApp();
            /* 不会返回 */
        }
        Debug_Print("[BOOT] App invalid, staying in bootloader\r\n");
    }
    
    /* 初始化 CAN 和 UDS */
    CAN_Init();
    UdsBootloader_Init();
    
    Debug_Print("[BOOT] Entering UDS main loop\r\n");
    Debug_Print("[BOOT] Waiting for UDS requests...\r\n");
    
    /* 主循环 */
    while (1) {
        /* 检查 CAN 接收 */
        if (CAN_Receive(&rxId, rxData, &rxDlc)) {
            /* 检查是否是 UDS 请求 */
            if (rxId == UDS_PHYS_REQUEST_ID || rxId == UDS_FUNC_REQUEST_ID) {
                /* 解析 ISO-TP 单帧 */
                if (ParseSingleFrame(rxData, rxDlc, gUdsRxBuffer, &gUdsRxLen)) {
                    /* 处理 UDS 请求 */
                    ProcessUdsRequest(gUdsRxBuffer, gUdsRxLen);
                }
            }
        }
        
        /* 慢闪 LED1 表示 Bootloader 运行中 */
        static volatile uint32_t counter = 0;
        counter++;
        if (counter > 500000) {
            counter = 0;
            LED_Toggle(1);
        }
    }
    
    return 0;
}
