# 更新日志

所有重要的更改都将记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 计划

- [ ] 添加 CAN FD 支持
- [ ] 完善 UDS 安全访问服务
- [x] ~~添加 Bootloader 功能~~ (已在 v1.1.0 完成)
- [ ] 支持更多诊断服务

## [1.1.0] - 2026-03-18

### 新增

#### Bootloader & OTA 升级
- 完整的 Bootloader 实现（64KB，起始地址 0x08000000）
- UDS 编程服务支持（$10/$11/$27/$31/$34/$36/$37）
- Flash 分区设计：64KB Bootloader + 960KB App
- App Valid Flag 机制（0x080FFFF0）
- Bootloader 到 App 的跳转逻辑
- App 跳转到 Bootloader 功能
- Python OTA 升级工具（`ota_updater.py`）

#### 新文件
- `bootloader/` 目录及全部源码
- `ota_updater.py` OTA 升级脚本
- `BootloaderJump.c/h` 跳转功能
- `STM32F407_Bootloader.ld` Bootloader 链接脚本

### 变更

- 更新 App 链接脚本，起始地址改为 0x08010000
- 更新 CMakeLists.txt，支持版本号定义（APP_VERSION_MAJOR/MINOR）
- 更新 README，添加 OTA 升级说明

## [1.0.0] - 2026-03-15

### 新增

#### AUTOSAR 协议栈
- 集成 CanIf 模块，提供 CAN 抽象接口
- 集成 PduR 模块，实现 PDU 路由功能
- 集成 Com 模块，支持信号路由和通信
- 集成 Dcm 模块，支持 UDS 诊断服务
- 集成 EcuM 模块，管理 ECU 状态

#### CAN 通信
- CAN1 驱动实现（PA11-RX, PA12-TX）
- 支持 500Kbps 波特率
- 支持标准帧（11位 ID）和扩展帧（29位 ID）
- CAN 接收中断处理
- CAN 过滤器配置

#### UDS 诊断服务
- 0x10 诊断会话控制
- 0x11 ECU 复位
- 0x22 读取 DID
- 0x2E 写入 DID
- 0x27 安全访问
- 0x3E Tester Present

#### 硬件支持
- STM32F407ZGT6 完整支持
- USART1 串口调试（115200bps）
- LED 控制（PE3, PE4, PG9）
- SysTick 1ms 定时器

#### 调试功能
- DebugLog 调试日志模块
- CAN 收发实时日志
- 系统运行状态日志
- 串口输出支持

#### 开发工具
- CMake 构建系统
- OpenOCD 调试支持
- J-Link/ST-Link 烧录脚本
- 自动化测试脚本

### 文档

- 完整的 README 说明
- API 文档（Doxygen 风格）
- 硬件连接图
- 使用示例

### 测试

- CAN 发送功能验证
- CAN 接收功能验证
- 串口输出验证
- LED 控制验证
- AUTOSAR 协议栈初始化验证

## [0.1.0] - 2026-03-10

### 新增

- 项目初始版本
- 基础 CAN 驱动
- 简单的 LED 控制
- 基础 CMake 配置

---

## 版本说明

### 版本号规则

- **主版本号**：不兼容的 API 更改
- **次版本号**：向下兼容的功能添加
- **修订号**：向下兼容的问题修复

### 标签说明

- `Added` 新添加的功能
- `Changed` 对现有功能的更改
- `Deprecated` 即将删除的功能
- `Removed` 已删除的功能
- `Fixed` 修复的 bug
- `Security` 安全相关的修复
