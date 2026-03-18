/**
 * @file        CanDriver.c
 * @brief       CAN1 驱动实现
 * @details     STM32F407 CAN1 外设驱动实现
 *              波特率计算: APB1 = 42MHz
 *              500Kbps = 42MHz / ((11 + 2 + 1) * 6) = 42MHz / 84
 *              采样点: (1+11)/(1+11+2) = 85.7%
 * 
 * @author      [小昭debug]
 * @date        2026-03-15
 */

#include "CanDriver.h"

/* 寄存器地址定义 */
#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRH      (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

#define CAN1_BASE       0x40006400
#define CAN1_MCR        (*(volatile uint32_t *)(CAN1_BASE + 0x000))
#define CAN1_MSR        (*(volatile uint32_t *)(CAN1_BASE + 0x004))
#define CAN1_TSR        (*(volatile uint32_t *)(CAN1_BASE + 0x008))
#define CAN1_RF0R       (*(volatile uint32_t *)(CAN1_BASE + 0x00C))
#define CAN1_RF1R       (*(volatile uint32_t *)(CAN1_BASE + 0x010))
#define CAN1_IER        (*(volatile uint32_t *)(CAN1_BASE + 0x014))
#define CAN1_BTR        (*(volatile uint32_t *)(CAN1_BASE + 0x01C))

#define CAN1_TI0R       (*(volatile uint32_t *)(CAN1_BASE + 0x180))
#define CAN1_TDT0R      (*(volatile uint32_t *)(CAN1_BASE + 0x184))
#define CAN1_TDL0R      (*(volatile uint32_t *)(CAN1_BASE + 0x188))
#define CAN1_TDH0R      (*(volatile uint32_t *)(CAN1_BASE + 0x18C))
#define CAN1_TI1R       (*(volatile uint32_t *)(CAN1_BASE + 0x190))
#define CAN1_TDT1R      (*(volatile uint32_t *)(CAN1_BASE + 0x194))
#define CAN1_TDL1R      (*(volatile uint32_t *)(CAN1_BASE + 0x198))
#define CAN1_TDH1R      (*(volatile uint32_t *)(CAN1_BASE + 0x19C))
#define CAN1_TI2R       (*(volatile uint32_t *)(CAN1_BASE + 0x1A0))
#define CAN1_TDT2R      (*(volatile uint32_t *)(CAN1_BASE + 0x1A4))
#define CAN1_TDL2R      (*(volatile uint32_t *)(CAN1_BASE + 0x1A8))
#define CAN1_TDH2R      (*(volatile uint32_t *)(CAN1_BASE + 0x1AC))

#define CAN1_RI0R       (*(volatile uint32_t *)(CAN1_BASE + 0x1B0))
#define CAN1_RDT0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1B4))
#define CAN1_RDL0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1B8))
#define CAN1_RDH0R      (*(volatile uint32_t *)(CAN1_BASE + 0x1BC))
#define CAN1_RI1R       (*(volatile uint32_t *)(CAN1_BASE + 0x1C0))
#define CAN1_RDT1R      (*(volatile uint32_t *)(CAN1_BASE + 0x1C4))
#define CAN1_RDL1R      (*(volatile uint32_t *)(CAN1_BASE + 0x1C8))
#define CAN1_RDH1R      (*(volatile uint32_t *)(CAN1_BASE + 0x1CC))

#define CAN1_FMR        (*(volatile uint32_t *)(CAN1_BASE + 0x200))
#define CAN1_FM1R       (*(volatile uint32_t *)(CAN1_BASE + 0x204))
#define CAN1_FS1R       (*(volatile uint32_t *)(CAN1_BASE + 0x20C))
#define CAN1_FFA1R      (*(volatile uint32_t *)(CAN1_BASE + 0x214))
#define CAN1_FA1R       (*(volatile uint32_t *)(CAN1_BASE + 0x21C))
#define CAN1_F0R1       (*(volatile uint32_t *)(CAN1_BASE + 0x240))
#define CAN1_F0R2       (*(volatile uint32_t *)(CAN1_BASE + 0x244))

#define NVIC_BASE       0xE000E100
#define NVIC_ISER0      (*(volatile uint32_t *)(NVIC_BASE + 0x000))

/* CAN MCR 寄存器位定义 */
#define CAN_MCR_INRQ    (1U << 0)
#define CAN_MCR_SLEEP   (1U << 1)
#define CAN_MCR_TXFP    (1U << 2)
#define CAN_MCR_RFLM    (1U << 3)
#define CAN_MCR_NART    (1U << 4)
#define CAN_MCR_AWUM    (1U << 5)
#define CAN_MCR_ABOM    (1U << 6)
#define CAN_MCR_TTCM    (1U << 7)
#define CAN_MCR_RESET   (1U << 15)
#define CAN_MCR_DBF     (1U << 16)

/* CAN MSR 寄存器位定义 */
#define CAN_MSR_INAK    (1U << 0)
#define CAN_MSR_SLAK    (1U << 1)
#define CAN_MSR_ERRI    (1U << 2)
#define CAN_MSR_WKUI    (1U << 3)
#define CAN_MSR_SLAKI   (1U << 4)

/* CAN TSR 寄存器位定义 */
#define CAN_TSR_RQCP0   (1U << 0)
#define CAN_TSR_TXOK0   (1U << 1)
#define CAN_TSR_TME0    (1U << 26)
#define CAN_TSR_TME1    (1U << 27)
#define CAN_TSR_TME2    (1U << 28)
#define CAN_TSR_TME     (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)

/* CAN RF0R 寄存器位定义 */
#define CAN_RF0R_FMP0   (0x3U << 0)
#define CAN_RF0R_FULL0  (1U << 3)
#define CAN_RF0R_FOVR0  (1U << 4)
#define CAN_RF0R_RFOM0  (1U << 5)

/* CAN IER 寄存器位定义 */
#define CAN_IER_TMEIE   (1U << 0)
#define CAN_IER_FMPIE0  (1U << 1)
#define CAN_IER_FFIE0   (1U << 2)
#define CAN_IER_FOVIE0  (1U << 3)
#define CAN_IER_FMPIE1  (1U << 4)

/* CAN BTR 寄存器位定义 */
#define CAN_BTR_BRP     (0x3FFU << 0)
#define CAN_BTR_TS1     (0x0FU << 16)
#define CAN_BTR_TS2     (0x07U << 20)
#define CAN_BTR_SJW     (0x03U << 24)
#define CAN_BTR_LBKM    (1U << 30)
#define CAN_BTR_SILM    (1U << 31)

/* TIxR 寄存器位定义 */
#define CAN_TIxR_TXRQ   (1U << 0)
#define CAN_TIxR_RTR    (1U << 1)
#define CAN_TIxR_IDE    (1U << 2)

/* 私有变量 */
static uint8_t gCanInitialized = 0;

CanStatus CanDriver_Init(void)
{
    uint32_t timeout;
    
    /* 防止重复初始化 */
    if (gCanInitialized) {
        return CAN_STATUS_OK;
    }
    
    /* 1. 使能 GPIOA 时钟 */
    RCC_AHB1ENR |= (1U << 0);
    
    /* 2. 配置 PA11 (CAN1_RX) 和 PA12 (CAN1_TX) 为复用功能 */
    /* PA11: 输入模式，复用功能 AF9 */
    GPIOA_MODER &= ~(3U << 22);
    GPIOA_MODER |= (2U << 22);
    
    /* PA12: 输出模式，复用功能 AF9 */
    GPIOA_MODER &= ~(3U << 24);
    GPIOA_MODER |= (2U << 24);
    
    /* 设置复用功能 AF9 (CAN1) */
    GPIOA_AFRH &= ~(0xFU << 12);
    GPIOA_AFRH |= (9U << 12);
    GPIOA_AFRH &= ~(0xFU << 16);
    GPIOA_AFRH |= (9U << 16);
    
    /* 3. 使能 CAN1 时钟 */
    RCC_APB1ENR |= (1U << 25);
    
    /* 4. 退出睡眠模式 */
    CAN1_MCR &= ~CAN_MCR_SLEEP;
    timeout = 10000;
    while ((CAN1_MSR & CAN_MSR_SLAK) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) {
        return CAN_STATUS_ERROR;
    }
    
    /* 5. 请求初始化模式 */
    CAN1_MCR |= CAN_MCR_INRQ;
    timeout = 10000;
    while ((!(CAN1_MSR & CAN_MSR_INAK)) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) {
        return CAN_STATUS_ERROR;
    }
    
    /* 6. 配置 CAN 工作模式 */
    /* 自动重传: 禁用 (NART=1)，自动唤醒: 禁用 */
    CAN1_MCR |= CAN_MCR_NART;
    
    /* 7. 配置波特率 500 Kbps (APB1 = 42MHz) */
    /* 500Kbps = 42MHz / ((11 + 2 + 1) * 6) = 42MHz / 84 */
    /* 采样点: (1+11)/(1+11+2) = 85.7% */
    CAN1_BTR = 0;
    CAN1_BTR |= (5U << 0);      /* BRP = 6-1 = 5 */
    CAN1_BTR |= (10U << 16);    /* TS1 = 11-1 = 10 */
    CAN1_BTR |= (1U << 20);     /* TS2 = 2-1 = 1 */
    CAN1_BTR |= (0U << 24);     /* SJW = 1 */
    
    /* 8. 请求正常模式 */
    CAN1_MCR &= ~CAN_MCR_INRQ;
    timeout = 10000;
    while ((CAN1_MSR & CAN_MSR_INAK) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) {
        return CAN_STATUS_ERROR;
    }
    
    /* 9. 配置过滤器 - 接收所有消息 */
    CAN1_FMR |= (1U << 0);          /* 过滤器初始化模式 */
    CAN1_FA1R &= ~(1U << 0);        /* 禁用过滤器 0 */
    
    /* 32位掩码模式，接收所有 ID */
    CAN1_FS1R |= (1U << 0);         /* 32位尺度 */
    CAN1_FM1R &= ~(1U << 0);        /* 掩码模式 */
    CAN1_FFA1R &= ~(1U << 0);       /* 分配到 FIFO 0 */
    
    CAN1_F0R1 = 0x00000000;         /* ID = 0 */
    CAN1_F0R2 = 0x00000000;         /* Mask = 0 (接收所有) */
    
    CAN1_FA1R |= (1U << 0);         /* 启用过滤器 0 */
    CAN1_FMR &= ~(1U << 0);         /* 退出过滤器初始化模式 */
    
    /* 10. 使能接收中断 */
    CAN1_IER |= CAN_IER_FMPIE0;
    
    /* 11. 配置 NVIC (CAN1 RX0 中断向量 20) */
    NVIC_ISER0 |= (1U << 20);
    
    gCanInitialized = 1;
    return CAN_STATUS_OK;
}

CanStatus CanDriver_Send(const CanMessage *msg, uint32_t timeout)
{
    uint32_t txMailbox;
    volatile uint32_t *tiReg;
    volatile uint32_t *tdtReg;
    volatile uint32_t *tdlReg;
    volatile uint32_t *tdhReg;
    uint32_t i;
    
    if (!gCanInitialized) {
        return CAN_STATUS_NOT_INIT;
    }
    
    if (msg == 0) {
        return CAN_STATUS_ERROR;
    }
    
    /* 检查数据长度 */
    if (msg->Dlc > 8) {
        return CAN_STATUS_ERROR;
    }
    
    /* 等待空闲邮箱 */
    i = 0;
    while (((CAN1_TSR & CAN_TSR_TME) == 0) && (i < timeout * 100)) {
        i++;
    }
    
    if ((CAN1_TSR & CAN_TSR_TME) == 0) {
        return CAN_STATUS_TX_FULL;
    }
    
    /* 选择邮箱 */
    if (CAN1_TSR & CAN_TSR_TME0) {
        txMailbox = 0;
        tiReg = &CAN1_TI0R;
        tdtReg = &CAN1_TDT0R;
        tdlReg = &CAN1_TDL0R;
        tdhReg = &CAN1_TDH0R;
    } else if (CAN1_TSR & CAN_TSR_TME1) {
        txMailbox = 1;
        tiReg = &CAN1_TI1R;
        tdtReg = &CAN1_TDT1R;
        tdlReg = &CAN1_TDL1R;
        tdhReg = &CAN1_TDH1R;
    } else {
        txMailbox = 2;
        tiReg = &CAN1_TI2R;
        tdtReg = &CAN1_TDT2R;
        tdlReg = &CAN1_TDL2R;
        tdhReg = &CAN1_TDH2R;
    }
    
    /* 设置 ID */
    *tiReg = 0;
    if (msg->IsExtId) {
        *tiReg = (msg->Id << 3) | CAN_TIxR_IDE;
    } else {
        *tiReg = (msg->Id << 21);
    }
    
    /* 远程帧 */
    if (msg->IsRemote) {
        *tiReg |= CAN_TIxR_RTR;
    }
    
    /* 设置数据长度 */
    *tdtReg = (msg->Dlc & 0x0F);
    
    /* 设置数据 */
    *tdlReg = ((uint32_t)msg->Data[3] << 24) |
              ((uint32_t)msg->Data[2] << 16) |
              ((uint32_t)msg->Data[1] << 8)  |
              ((uint32_t)msg->Data[0]);
    
    *tdhReg = ((uint32_t)msg->Data[7] << 24) |
              ((uint32_t)msg->Data[6] << 16) |
              ((uint32_t)msg->Data[5] << 8)  |
              ((uint32_t)msg->Data[4]);
    
    /* 请求发送 */
    *tiReg |= CAN_TIxR_TXRQ;
    
    /* 等待发送完成 (非阻塞模式可省略) */
    if (timeout > 0) {
        i = 0;
        while ((!(CAN1_TSR & (CAN_TSR_RQCP0 << (txMailbox * 8)))) && 
               (i < timeout * 100)) {
            i++;
        }
        
        if (!(CAN1_TSR & (CAN_TSR_RQCP0 << (txMailbox * 8)))) {
            return CAN_STATUS_BUSY;
        }
        
        /* 检查是否发送成功 */
        if (!(CAN1_TSR & (CAN_TSR_TXOK0 << (txMailbox * 8)))) {
            return CAN_STATUS_ERROR;
        }
    }
    
    return CAN_STATUS_OK;
}

CanStatus CanDriver_Receive(CanMessage *msg)
{
    uint32_t riReg;
    uint32_t rdtReg;
    uint32_t rdlReg;
    uint32_t rdhReg;
    uint32_t i;
    
    if (!gCanInitialized) {
        return CAN_STATUS_NOT_INIT;
    }
    
    if (msg == 0) {
        return CAN_STATUS_ERROR;
    }
    
    /* 检查 FIFO 0 是否有消息 */
    if ((CAN1_RF0R & CAN_RF0R_FMP0) == 0) {
        return CAN_STATUS_RX_EMPTY;
    }
    
    /* 读取寄存器 */
    riReg = CAN1_RI0R;
    rdtReg = CAN1_RDT0R;
    rdlReg = CAN1_RDL0R;
    rdhReg = CAN1_RDH0R;
    
    /* 解析 ID */
    if (riReg & CAN_TIxR_IDE) {
        msg->IsExtId = 1;
        msg->Id = (riReg >> 3) & 0x1FFFFFFF;
    } else {
        msg->IsExtId = 0;
        msg->Id = (riReg >> 21) & 0x7FF;
    }
    
    /* 解析远程帧 */
    msg->IsRemote = (riReg & CAN_TIxR_RTR) ? 1 : 0;
    
    /* 解析数据长度 */
    msg->Dlc = (rdtReg >> 0) & 0x0F;
    if (msg->Dlc > 8) {
        msg->Dlc = 8;
    }
    
    /* 解析数据 */
    for (i = 0; i < msg->Dlc; i++) {
        if (i < 4) {
            msg->Data[i] = (uint8_t)(rdlReg >> (i * 8));
        } else {
            msg->Data[i] = (uint8_t)(rdhReg >> ((i - 4) * 8));
        }
    }
    
    /* 释放 FIFO */
    CAN1_RF0R |= CAN_RF0R_RFOM0;
    
    return CAN_STATUS_OK;
}

uint8_t CanDriver_IsMessagePending(void)
{
    if (!gCanInitialized) {
        return 0;
    }
    return ((CAN1_RF0R & CAN_RF0R_FMP0) != 0) ? 1 : 0;
}

CanStatus CanDriver_SetFilter(uint32_t filterId, uint32_t mask, uint8_t isExtId)
{
    if (!gCanInitialized) {
        return CAN_STATUS_NOT_INIT;
    }
    
    /* 进入过滤器初始化模式 */
    CAN1_FMR |= (1U << 0);
    CAN1_FA1R &= ~(1U << 0);
    
    /* 32位尺度 */
    CAN1_FS1R |= (1U << 0);
    
    /* 掩码模式 */
    CAN1_FM1R &= ~(1U << 0);
    
    /* 分配到 FIFO 0 */
    CAN1_FFA1R &= ~(1U << 0);
    
    /* 设置 ID 和掩码 */
    if (isExtId) {
        CAN1_F0R1 = (filterId << 3) | CAN_TIxR_IDE;
        CAN1_F0R2 = (mask << 3) | CAN_TIxR_IDE;
    } else {
        CAN1_F0R1 = (filterId << 21);
        CAN1_F0R2 = (mask << 21);
    }
    
    /* 启用过滤器 */
    CAN1_FA1R |= (1U << 0);
    
    /* 退出过滤器初始化模式 */
    CAN1_FMR &= ~(1U << 0);
    
    return CAN_STATUS_OK;
}

/*==================================================================================================
 *                                      中断处理
==================================================================================================*/

/* 前向声明CanIf_RxIndication (仅在 AUTOSAR 模式下使用) */
#ifdef ENABLE_AUTOSAR
extern void CanIf_RxIndication(uint8_t Hrh, uint32_t CanId, uint8_t CanDlc, const uint8_t* CanSduPtr);
#endif

/**
 * @brief  CAN1 RX0中断服务程序
 * @note   处理CAN接收FIFO 0的中断
 */
void CAN1_RX0_IRQHandler(void)
{
    CanMessage rxMsg;
    uint8_t data[8];
    uint8_t i;
    
    /* 检查FIFO 0是否有消息 */
    if ((CAN1_RF0R & CAN_RF0R_FMP0) == 0) {
        return;
    }
    
    /* 读取消息 */
    rxMsg.Id = CAN1_RI0R;
    rxMsg.Dlc = CAN1_RDT0R & 0x0F;
    
    /* 解析ID和类型 */
    if (rxMsg.Id & CAN_TIxR_IDE) {
        rxMsg.IsExtId = 1;
        rxMsg.Id = (rxMsg.Id >> 3) & 0x1FFFFFFF;
    } else {
        rxMsg.IsExtId = 0;
        rxMsg.Id = (rxMsg.Id >> 21) & 0x7FF;
    }
    
    /* 解析远程帧 */
    rxMsg.IsRemote = (CAN1_RI0R & CAN_TIxR_RTR) ? 1 : 0;
    
    /* 限制数据长度 */
    if (rxMsg.Dlc > 8) {
        rxMsg.Dlc = 8;
    }
    
    /* 读取数据 */
    {
        uint32_t rdl = CAN1_RDL0R;
        uint32_t rdh = CAN1_RDH0R;
        
        for (i = 0; i < rxMsg.Dlc; i++) {
            if (i < 4) {
                data[i] = (uint8_t)(rdl >> (i * 8));
            } else {
                data[i] = (uint8_t)(rdh >> ((i - 4) * 8));
            }
        }
    }
    
    /* 释放FIFO */
    CAN1_RF0R |= CAN_RF0R_RFOM0;
    
#ifdef ENABLE_AUTOSAR
    /* 调用CanIf接收指示 */
    CanIf_RxIndication(0, rxMsg.Id, rxMsg.Dlc, data);
#endif
}
