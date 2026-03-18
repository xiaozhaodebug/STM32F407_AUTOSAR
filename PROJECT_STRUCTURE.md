# 项目结构说明

本文档描述 STM32F407 AUTOSAR 开源项目的目录结构。

## 目录树

```
STM32F407_AUTOSAR/
├── AUTOSAR/                  # AUTOSAR 协议栈实现
│   ├── CanIf/               # CAN 接口层
│   │   ├── CanIf.c
│   │   └── CanIf.h
│   ├── Com/                 # 通信服务层
│   │   ├── Com.c
│   │   └── Com.h
│   ├── Dcm/                 # 诊断通信管理
│   │   ├── Dcm.c
│   │   └── Dcm.h
│   ├── EcuM/                # ECU 状态管理
│   │   ├── EcuM.c
│   │   └── EcuM.h
│   ├── PduR/                # PDU 路由
│   │   ├── PduR.c
│   │   └── PduR.h
│   ├── Std/                 # 标准类型定义
│   │   ├── Std_Types.c
│   │   ├── Std_Types.h
│   │   └── ComStack_Types.h
│   ├── AUTOSAR_Cfg.h        # 配置文件
│   └── AUTOSAR_Integration.c # 集成代码
├── bootloader/              # Bootloader 目录 (OTA 升级)
│   ├── src/                 # Bootloader 源码
│   │   ├── main_bootloader.c    # Bootloader 主程序
│   │   ├── boot.c               # 启动跳转逻辑
│   │   ├── flash_drv.c          # Flash 驱动
│   │   ├── crc32.c              # CRC 校验
│   │   └── uds_bootloader.c     # UDS 编程服务
│   ├── include/             # Bootloader 头文件
│   │   ├── boot.h
│   │   ├── flash_drv.h
│   │   ├── crc32.h
│   │   └── uds_bootloader.h
│   ├── ld/                  # Bootloader 链接脚本
│   │   └── STM32F407_Bootloader.ld
│   └── CMakeLists.txt       # Bootloader 构建配置
├── src/                      # 源代码
│   ├── board/               # 板级支持
│   │   ├── main.c           # 主程序
│   │   ├── startup_stm32f407xx.s  # 启动文件
│   │   └── system_stm32f4xx.c     # 系统时钟配置
│   ├── drivers/             # 设备驱动
│   │   └── CanDriver.c      # CAN 驱动
│   └── utils/               # 工具函数
│       ├── DebugLog.c       # 调试日志
│       └── BootloaderJump.c # 跳转到 Bootloader
├── include/                  # 头文件
│   ├── CanDriver.h
│   ├── DebugLog.h
│   └── BootloaderJump.h     # 跳转功能头文件
├── ld/                       # 链接脚本
│   ├── STM32F407ZGTx_FLASH.ld       # App 链接脚本 (0x08010000)
│   └── STM32F407_Bootloader.ld      # Bootloader 链接脚本 (0x08000000)
├── docs/                     # 文档
│   ├── architecture.md      # 架构说明
│   ├── api.md               # API 参考
│   └── hardware.md          # 硬件连接指南
├── examples/                 # 示例代码
│   ├── example_can_tx.c     # CAN 发送示例
│   ├── example_can_rx.c     # CAN 接收示例
│   ├── example_uds.c        # UDS 诊断示例
│   └── README.md
├── tools/                    # 工具脚本
│   ├── flash.sh             # 烧录脚本
│   ├── debug.sh             # 调试脚本
│   ├── setup_can.sh         # CAN 配置脚本
│   └── code_generators/     # 代码生成器
│       ├── dbc_generator.py
│       └── uds_generator/
├── ota_updater.py           # OTA 升级 Python 工具
├── tests/                    # 测试代码
├── build/                    # 构建目录 (自动生成)
├── README.md                 # 项目说明
├── LICENSE                   # 许可证
├── CONTRIBUTING.md           # 贡献指南
├── CHANGELOG.md              # 更新日志
├── PROJECT_STRUCTURE.md      # 本文件
└── CMakeLists.txt            # CMake 配置
```

## 目录说明

### AUTOSAR/

AUTOSAR 协议栈的核心实现，包含以下模块：

- **CanIf**: CAN 接口抽象层，提供统一的 CAN 访问接口
- **PduR**: PDU 路由层，负责 PDU 的路由和分发
- **Com**: 通信服务层，提供信号路由和通信功能
- **Dcm**: 诊断通信管理，实现 UDS 诊断服务
- **EcuM**: ECU 状态管理，管理 ECU 状态和初始化序列
- **Std**: 标准类型定义，包含 AUTOSAR 标准类型和宏定义

### bootloader/

Bootloader 目录，用于 OTA（Over-The-Air）远程固件升级：

- **src/**: 源码文件
  - `main_bootloader.c`: Bootloader 主程序（CAN 接收 + UDS 处理）
  - `boot.c`: 启动逻辑（App 验证、跳转）
  - `flash_drv.c`: Flash 读写驱动
  - `crc32.c`: CRC32 校验算法
  - `uds_bootloader.c`: UDS 编程服务（$34/$36/$37）
- **include/**: 头文件
- **ld/**: Bootloader 专用链接脚本（起始地址 0x08000000）
- **CMakeLists.txt**: Bootloader 构建配置

### src/

源代码目录，包含：

- **board/**: 板级支持代码
  - `main.c`: 主程序入口
  - `startup_stm32f407xx.s`: 启动汇编文件
  - `system_stm32f4xx.c`: 系统时钟配置

- **drivers/**: 设备驱动代码
  - `CanDriver.c`: CAN1 硬件驱动实现

- **utils/**: 工具函数
  - `DebugLog.c`: 调试日志输出模块
  - `BootloaderJump.c`: 跳转到 Bootloader 功能

### src/

源代码目录，包含：

- **board/**: 板级支持代码
  - `main.c`: 主程序入口
  - `startup_stm32f407xx.s`: 启动汇编文件
  - `system_stm32f4xx.c`: 系统时钟配置

- **drivers/**: 设备驱动代码
  - `CanDriver.c`: CAN1 硬件驱动实现

- **utils/**: 工具函数
  - `DebugLog.c`: 调试日志输出模块

### include/

公共头文件目录，包含：

- `CanDriver.h`: CAN 驱动头文件
- `DebugLog.h`: 调试日志头文件
- `BootloaderJump.h`: 跳转到 Bootloader 功能头文件
- `DbcConfig.h`: DBC 配置头文件（代码生成器生成）

### docs/

项目文档目录：

- `architecture.md`: 系统架构说明
- `api.md`: API 参考文档
- `hardware.md`: 硬件连接指南

### examples/

示例代码目录，展示如何使用项目功能：

- `example_can_tx.c`: CAN 发送示例
- `example_can_rx.c`: CAN 接收示例
- `example_uds.c`: UDS 诊断示例

### tools/

实用工具脚本：

- `flash.sh`: 固件烧录脚本
- `debug.sh`: 调试脚本
- `setup_can.sh`: CAN 接口配置脚本
- `code_generators/`: 代码生成器工具
  - `dbc_generator.py`: DBC 代码生成器
  - `uds_generator/`: UDS 配置生成器

### ld/

链接脚本目录，包含：

- `STM32F407ZGTx_FLASH.ld`: App 链接脚本（起始地址 0x08010000）
- `STM32F407_Bootloader.ld`: Bootloader 链接脚本（起始地址 0x08000000）

### build/

构建输出目录（由 CMake 自动生成），包含编译生成的文件：

- `STM32F407_CAN.elf`: ELF 格式固件
- `STM32F407_CAN.hex`: HEX 格式固件
- `STM32F407_CAN.bin`: BIN 格式固件

## 文件命名规范

### 源代码文件

- 驱动文件: `<Device>Driver.c` (如 `CanDriver.c`)
- 模块文件: `<Module>.c` (如 `CanIf.c`)
- 板级文件: `main.c`, `system_stm32f4xx.c`
- 启动文件: `startup_stm32f407xx.s`

### 头文件

- 驱动头文件: `<Device>Driver.h` (如 `CanDriver.h`)
- 模块头文件: `<Module>.h` (如 `CanIf.h`)
- 配置头文件: `AUTOSAR_Cfg.h`

### 文档文件

- 说明文档: `README.md`
- 架构文档: `architecture.md`
- API 文档: `api.md`
- 硬件文档: `hardware.md`

## 编码规范

### 目录组织原则

1. **模块化**: 每个 AUTOSAR 模块独立目录
2. **分层**: 驱动层、板级层、应用层分离
3. **清晰**: 示例代码和工具脚本单独存放
4. **完整**: 文档齐全，便于理解和使用

### 头文件路径

编译时的头文件搜索路径：

```
-I include
-I src/drivers
-I src/utils
-I AUTOSAR
-I AUTOSAR/Std
-I AUTOSAR/CanIf
-I AUTOSAR/PduR
-I AUTOSAR/Com
-I AUTOSAR/Dcm
-I AUTOSAR/EcuM
```

## 构建系统

项目使用 CMake 构建系统，主要配置在 `CMakeLists.txt` 中：

- 交叉编译器: `arm-none-eabi-gcc`
- 目标架构: Cortex-M4
- 浮点单元: FPv4-SP-D16 (硬件浮点)
- 链接脚本: `ld/STM32F407ZGTx_FLASH.ld`

## 版本控制

`.gitignore` 文件已配置，忽略以下目录：

- `build/`: 构建输出
- `.vscode/`, `.idea/`: IDE 配置
- `*.o`, `*.elf`, `*.bin`, `*.hex`: 编译生成文件

## 根目录文件

- `README.md`: 项目主说明文档
- `README_CN.md`: 中文说明文档
- `LICENSE`: MIT 许可证
- `CONTRIBUTING.md`: 贡献指南
- `CHANGELOG.md`: 版本更新日志
- `PROJECT_STRUCTURE.md`: 本文件（项目结构说明）
- `HARDWARE_SPEC.md`: 硬件规格说明
- `CMakeLists.txt`: CMake 构建配置
- `ota_updater.py`: OTA 升级 Python 工具（UDS $34/$36/$37 刷写）

## 扩展建议

添加新功能时，建议按以下方式组织：

1. **新驱动**: 添加到 `src/drivers/`，头文件添加到 `include/`
2. **新模块**: 在 `AUTOSAR/` 下创建新目录
3. **新示例**: 添加到 `examples/`
4. **新文档**: 添加到 `docs/`
5. **新工具**: 添加到 `tools/`
6. **Bootloader 功能**: 在 `bootloader/src/` 添加，并更新 `bootloader/include/` 头文件
