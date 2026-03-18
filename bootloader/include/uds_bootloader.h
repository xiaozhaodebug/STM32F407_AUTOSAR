/**
 * @file    uds_bootloader.h
 * @brief   UDS Bootloader 升级服务头文件
 * @details 实现 ISO 14229 编程服务
 * 
 * @author  STM32_AUTOSAR Project
 * @date    2026-03-18
 */

#ifndef UDS_BOOTLOADER_H
#define UDS_BOOTLOADER_H

#include <stdint.h>
#include "Std_Types.h"

/*==============================================================================
 *                              宏定义
 *============================================================================*/

/* 例程控制 ID */
#define ROUTINE_ERASE_MEMORY        0xFF00
#define ROUTINE_CHECK_INTEGRITY     0xFF01
#define ROUTINE_CHECK_DEPENDENCIES  0xFF02

/* 编程状态 */
#define UDS_PROG_STATE_IDLE         0
#define UDS_PROG_STATE_PRE          1
#define UDS_PROG_STATE_PROGRAMMING  2
#define UDS_PROG_STATE_POST         3

/* 下载状态 */
#define DCM_DOWNLOAD_IDLE           0
#define DCM_DOWNLOAD_STARTED        1
#define DCM_DOWNLOAD_COMPLETED      2

/* Flash 缓冲区大小 */
#define FLASH_BUFFER_SIZE           1024

/*==============================================================================
 *                              类型定义
 *============================================================================*/

typedef struct {
    uint8_t state;              /**< 编程状态 */
    uint32_t downloadAddress;   /**< 下载起始地址 */
    uint32_t downloadSize;      /**< 下载总大小 */
    uint32_t receivedSize;      /**< 已接收大小 */
    uint32_t currentAddr;       /**< 当前写入地址 */
    uint8_t blockCounter;       /**< 块序号 */
} Uds_DownloadStateType;

/*==============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief  初始化 UDS Bootloader 服务
 */
void UdsBootloader_Init(void);

/**
 * @brief  处理 RequestDownload ($34) 服务
 */
Std_ReturnType UdsService_RequestDownload(
    const uint8_t *requestData,
    uint16_t requestLen,
    uint8_t *responseData,
    uint16_t *responseLen);

/**
 * @brief  处理 TransferData ($36) 服务
 */
Std_ReturnType UdsService_TransferData(
    const uint8_t *requestData,
    uint16_t requestLen,
    uint8_t *responseData,
    uint16_t *responseLen);

/**
 * @brief  处理 RequestTransferExit ($37) 服务
 */
Std_ReturnType UdsService_RequestTransferExit(
    const uint8_t *requestData,
    uint16_t requestLen,
    uint8_t *responseData,
    uint16_t *responseLen);

/**
 * @brief  处理 RoutineControl ($31) 服务 - 擦除内存
 */
Std_ReturnType UdsService_RoutineEraseMemory(
    const uint8_t *requestData,
    uint16_t requestLen,
    uint8_t *responseData,
    uint16_t *responseLen);

/**
 * @brief  处理 RoutineControl ($31) 服务 - 检查完整性
 */
Std_ReturnType UdsService_RoutineCheckIntegrity(
    const uint8_t *requestData,
    uint16_t requestLen,
    uint8_t *responseData,
    uint16_t *responseLen);

/**
 * @brief  完成编程，设置 App 有效
 */
void UdsBootloader_FinishProgramming(void);

/**
 * @brief  获取下载状态
 */
const Uds_DownloadStateType* UdsBootloader_GetDownloadState(void);

/**
 * @brief  重置下载状态
 */
void UdsBootloader_ResetDownloadState(void);

#endif /* UDS_BOOTLOADER_H */
