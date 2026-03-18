/**
 * @file    main.c
 * @brief   STM32F407 App with UDS Support
 * @details 支持 UDS 诊断服务，包括跳转到 Bootloader 进行 OTA
 * 
 * @author  [小昭debug]
 * @date    2026-03-18
 */

#include <stdint.h>
#include <string.h>
#include "DebugLog.h"
#include "CanDriver.h"
#include "DbcConfig.h"
#include "BootloaderJump.h"

/* 寄存器定义 */
#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))

#define GPIOE_BASE      0x40021000
#define GPIOE_MODER     (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_BSRR      (*(volatile uint32_t *)(GPIOE_BASE + 0x18))

#define GPIOG_BASE      0x40021800
#define GPIOG_MODER     (*(volatile uint32_t *)(GPIOG_BASE + 0x00))
#define GPIOG_BSRR      (*(volatile uint32_t *)(GPIOG_BASE + 0x18))

#define SYSTICK_BASE    0xE000E010
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL     (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

/* SCB - 系统控制块 */
#define SCB_CPUID       (*(volatile uint32_t *)0xE000ED00)

/* 版本定义 */
#ifndef APP_VERSION_MAJOR
#define APP_VERSION_MAJOR   1
#endif
#ifndef APP_VERSION_MINOR
#define APP_VERSION_MINOR   0
#endif

/* UDS CAN ID */
#define UDS_PHYS_REQUEST_ID     0x735
#define UDS_FUNC_REQUEST_ID     0x7DF
#define UDS_RESPONSE_ID         0x73D

/* 全局变量 */
static volatile uint32_t gTickCount = 0;
static uint8_t gUdsSessionType = 0x01;  /* 默认会话 */
static uint8_t gUdsSecurityLevel = 0;   /* 未解锁 */

/*==============================================================================
 *                              GPIO / LED
 *============================================================================*/

static void GpioInit(void)
{
    /* 使能 GPIOE 和 GPIOG 时钟 */
    RCC_AHB1ENR |= (1U << 4);  /* GPIOE */
    RCC_AHB1ENR |= (1U << 6);  /* GPIOG */
    
    /* PE3, PE4 配置为输出 */
    GPIOE_MODER &= ~((3U << 6) | (3U << 8));
    GPIOE_MODER |= ((1U << 6) | (1U << 8));
    
    /* PG9 配置为输出 */
    GPIOG_MODER &= ~(3U << 18);
    GPIOG_MODER |= (1U << 18);
    
    /* 关闭所有 LED (输出高电平) */
    GPIOE_BSRR = ((1U << 3) | (1U << 4));
    GPIOG_BSRR = (1U << 9);
}

static void LedSet(uint8_t led, uint8_t on)
{
    switch (led) {
        case 0: /* PE3 */
            if (on) GPIOE_BSRR = (1U << (3 + 16));
            else    GPIOE_BSRR = (1U << 3);
            break;
        case 1: /* PE4 */
            if (on) GPIOE_BSRR = (1U << (4 + 16));
            else    GPIOE_BSRR = (1U << 4);
            break;
        case 2: /* PG9 */
            if (on) GPIOG_BSRR = (1U << (9 + 16));
            else    GPIOG_BSRR = (1U << 9);
            break;
    }
}

static void LedToggle(uint8_t led)
{
    static uint8_t state[3] = {0, 0, 0};
    state[led] = !state[led];
    LedSet(led, state[led]);
}

/*==============================================================================
 *                              SysTick
 *============================================================================*/

static void SysTick_Init(void)
{
    SYSTICK_LOAD = 168000 - 1;  /* 1ms @ 168MHz */
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x07;  /* 使能中断，使用处理器时钟 */
}

void SysTick_Handler(void)
{
    gTickCount++;
}

uint32_t GetTick(void)
{
    return gTickCount;
}

/*==============================================================================
 *                              UDS 服务
 *============================================================================*/

static uint8_t gUdsRxBuffer[256];
static uint16_t gUdsRxLen = 0;

/**
 * @brief  解析 ISO-TP 单帧
 */
static uint8_t ParseSingleFrame(const uint8_t *data, uint8_t dlc, uint8_t *outData, uint16_t *outLen)
{
    if (dlc < 1) return 0;
    
    uint8_t len = data[0] & 0x0F;
    if (len > 7 || len + 1 > dlc) return 0;
    
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
    for (uint8_t i = len + 1; i < 8; i++) {
        outData[i] = 0x00;
    }
    *outDlc = 8;
}

/**
 * @brief  发送 UDS 响应
 */
static void UdsSendResponse(const uint8_t *data, uint8_t len)
{
    uint8_t txData[8];
    uint8_t dlc;
    BuildSingleFrame(data, len, txData, &dlc);
    
    CanMessage txMsg;
    txMsg.Id = UDS_RESPONSE_ID;
    txMsg.IsExtId = 0;
    txMsg.IsRemote = 0;
    txMsg.Dlc = dlc;
    for (uint8_t i = 0; i < 8; i++) {
        txMsg.Data[i] = txData[i];
    }
    CanDriver_Send(&txMsg, 10);
}

/**
 * @brief  发送否定响应
 */
static void UdsSendNrc(uint8_t sid, uint8_t nrc)
{
    uint8_t response[3];
    response[0] = 0x7F;
    response[1] = sid;
    response[2] = nrc;
    UdsSendResponse(response, 3);
}

/**
 * @brief  处理 UDS 请求
 */
static void ProcessUdsRequest(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    
    uint8_t sid = data[0];
    uint8_t response[256];
    uint8_t responseLen = 0;
    
    switch (sid) {
        case 0x10:  /* DiagnosticSessionControl */
            if (len >= 2) {
                gUdsSessionType = data[1];
                response[0] = 0x50;
                response[1] = gUdsSessionType;
                response[2] = 0x00;
                response[3] = 0x32;
                response[4] = 0x01;
                response[5] = 0xF4;
                UdsSendResponse(response, 6);
                DebugLog_String("[UDS] Session changed to ");
                DebugLog_Dec(gUdsSessionType);
                DebugLog_NewLine();
            } else {
                UdsSendNrc(sid, 0x13);
            }
            break;
            
        case 0x11:  /* ECUReset */
            if (len >= 2) {
                uint8_t resetType = data[1];
                response[0] = 0x51;
                response[1] = resetType;
                UdsSendResponse(response, 2);
                DebugLog_String("[UDS] ECU Reset type=");
                DebugLog_Dec(resetType);
                DebugLog_NewLine();
                
                /* 延时后复位 */
                for (volatile uint32_t i = 0; i < 1000000; i++);
                
                /* 执行复位 */
                *(volatile uint32_t *)0xE000ED0C = 0x05FA0004;
            }
            break;
            
        case 0x22:  /* ReadDataByIdentifier */
            if (len >= 3) {
                uint16_t did = (data[1] << 8) | data[2];
                response[0] = 0x62;
                response[1] = data[1];
                response[2] = data[2];
                
                if (did == 0xF180) {  /* App 版本 */
                    response[3] = APP_VERSION_MAJOR;
                    response[4] = APP_VERSION_MINOR;
                    UdsSendResponse(response, 5);
                } else if (did == 0xF197) {  /* 系统名称 */
                    const char *name = "STM32F407";
                    memcpy(&response[3], name, 9);
                    UdsSendResponse(response, 12);
                } else if (did == 0xF194) {  /* Bootloader 版本 */
                    response[3] = 0x01;
                    response[4] = 0x00;
                    UdsSendResponse(response, 5);
                } else {
                    UdsSendNrc(sid, 0x31);
                }
            }
            break;
            
        case 0x3E:  /* TesterPresent */
            response[0] = 0x7E;
            if (len >= 2 && data[1] == 0x80) {
                /* 抑制响应 */
            } else {
                UdsSendResponse(response, 1);
            }
            break;
            
        default:
            UdsSendNrc(sid, 0x11);  /* 服务不支持 */
            break;
    }
}

/*==============================================================================
 *                              CAN 处理
 *============================================================================*/

static void ProcessCanMessage(const CanMessage *rxMsg)
{
    /* 检查是否是 UDS 请求 */
    if (rxMsg->Id == UDS_PHYS_REQUEST_ID || rxMsg->Id == UDS_FUNC_REQUEST_ID) {
        uint16_t udsLen;
        if (ParseSingleFrame(rxMsg->Data, rxMsg->Dlc, gUdsRxBuffer, &udsLen)) {
            ProcessUdsRequest(gUdsRxBuffer, udsLen);
        }
        return;
    }
    
    /* 处理 DBC 相关消息 */
    switch (rxMsg->Id) {
        case MSG_ID_XZ_B_MOTORSTATE:
            /* 处理电机状态 */
            break;
        case MSG_ID_XZ_B_CTRL_STATE:
            /* 处理控制状态 */
            break;
    }
}

/*==============================================================================
 *                              主函数
 *============================================================================*/

int main(void)
{
    /* 检查是否从 Bootloader 跳转过来 */
    if (BootloaderJump_CheckBootFromLoader()) {
        BootloaderJump_ClearFlag();
        DebugLog_String("[APP] Booted from Bootloader\r\n");
    }
    
    /* 初始化 */
    GpioInit();
    DebugLog_Init();
    
    DebugLog_String("\r\n================================\r\n");
    DebugLog_String("STM32F407 AUTOSAR App\r\n");
    DebugLog_String("Version: ");
    DebugLog_Dec(APP_VERSION_MAJOR);
    DebugLog_String(".");
    DebugLog_Dec(APP_VERSION_MINOR);
    DebugLog_String("\r\n");
    DebugLog_String("UDS Support: Enabled\r\n");
    DebugLog_String("================================\r\n");
    
    /* 初始化 CAN */
    DebugLog_String("[APP] Initializing CAN...\r\n");
    CanStatus status = CanDriver_Init();
    if (status == CAN_STATUS_OK) {
        DebugLog_String("[APP] CAN Init OK\r\n");
        LedSet(0, 1);  /* LED0 点亮表示 CAN 正常 */
    } else {
        DebugLog_String("[APP] CAN Init Failed\r\n");
    }
    
    /* 初始化 SysTick */
    SysTick_Init();
    
    /* 初始化 DBC 信号 */
    XZ_A_Led_State_t ledState = {0};
    uint8_t canTxData[8];
    
    ledState.Led1_State = 1;
    ledState.Led2_State = 2;
    
    DebugLog_String("[APP] Entering main loop\r\n");
    
    uint32_t lastLedTx = 0;
    uint32_t lastStatusPrint = 0;
    
    /* 主循环 */
    while (1) {
        /* 接收处理 */
        if (CanDriver_IsMessagePending()) {
            CanMessage rxMsg;
            if (CanDriver_Receive(&rxMsg) == CAN_STATUS_OK) {
                ProcessCanMessage(&rxMsg);
            }
        }
        
        /* 发送状态报文 (每 100ms) */
        if ((gTickCount - lastLedTx) >= 100) {
            lastLedTx = gTickCount;
            
            /* 更新 LED 状态 */
            ledState.Led2_State = ((gTickCount / 500) % 2) ? 2 : 0;
            
            /* 打包发送 */
            Dbc_Pack_XZ_A_Led_State(canTxData, &ledState);
            
            CanMessage txMsg;
            txMsg.Id = MSG_ID_XZ_A_LED_STATE;
            txMsg.IsExtId = 0;
            txMsg.IsRemote = 0;
            txMsg.Dlc = 8;
            for (uint8_t i = 0; i < 8; i++) {
                txMsg.Data[i] = canTxData[i];
            }
            CanDriver_Send(&txMsg, 10);
        }
        
        /* 状态打印 (每 5秒) */
        if ((gTickCount - lastStatusPrint) >= 5000) {
            lastStatusPrint = gTickCount;
            DebugLog_String("[APP] Running... Tick=");
            DebugLog_Dec(gTickCount);
            DebugLog_NewLine();
            LedToggle(1);  /* LED1 闪烁 */
        }
        
        /* 低功耗等待 */
        __asm__ volatile("wfi");
    }
    
    return 0;
}
