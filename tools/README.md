# STM32 AUTOSAR 工具集

本目录包含 STM32 AUTOSAR 项目的开发工具，按功能分类整理。

## 📁 目录结构

```
tools/
├── README.md                    # 本文件
├── code_generators/             # 代码生成器
├── build_tools/                 # 构建烧录工具
├── debug_tools/                 # 调试工具
├── diagnostic_tools/            # 诊断测试工具
└── config/                      # 配置文件
```

---

## 🔧 1. code_generators - 代码生成器

### 1.1 DBC 代码生成器

从 Excel DBC 配置表自动生成 C 代码。

**文件：**
- `dbc_generator.py` - Python 生成器主脚本
- `generate_dbc.sh` - 一键生成脚本（推荐）
- `README_DBC_GENERATOR.md` - 详细文档

**使用方法：**

```bash
cd code_generators

# 方式1：使用一键脚本（推荐）
./generate_dbc.sh

# 方式2：使用 Python 直接运行
python3 dbc_generator.py ../config/XZ_CAN_V2.xlsx
```

**生成文件：**
- `DbcConfig.h` - DBC 配置头文件
- `DbcHandler.c/h` - DBC 处理函数

**Excel 配置表示例：**

| Message ID | Name | Length | Cycle(ms) | Signal Name | Start Bit | Length | Factor | Offset |
|------------|------|--------|-----------|-------------|-----------|--------|--------|--------|
| 0x210 | Led_State | 8 | 50 | Led1_State | 0 | 2 | 1 | 0 |
| 0x211 | Key_State | 8 | 20 | Key1_State | 0 | 4 | 1 | 0 |

---

### 1.2 UDS 诊断配置生成器

从 Excel 配置表生成 UDS 诊断代码，支持 Session、DID、Security、Routine、DTC 配置。

**文件：**
- `uds_generator/uds_generator.py` - Python 生成器
- `uds_generator/generate_uds.sh` - 一键生成脚本
- `uds_generator/README.md` - 详细文档

**使用方法：**

```bash
cd code_generators/uds_generator

# 第1步：创建示例配置
./generate_uds.sh --example
# 生成 UDS_Config_Example.xlsx

# 第2步：编辑配置表
# 使用 Excel 编辑 UDS_Config_Example.xlsx

# 第3步：生成代码
./generate_uds.sh UDS_Config_Example.xlsx
```

**配置工作表说明：**

| 工作表 | 说明 | 对应UDS服务 |
|--------|------|-------------|
| SessionConfig | 会话配置（P2/S3时间参数） | 0x10 |
| DIDConfig | 数据标识符配置 | 0x22/0x2E |
| SecurityConfig | 安全访问配置 | 0x27 |
| RoutineConfig | 例行程序配置 | 0x31 |
| DTCConfig | 故障码配置 | 0x19 |
| Communication | CAN ID和PDU配置 | - |

**SessionConfig 示例：**

| SessionType | Name | P2_max | P2*_max | S3_server | SupportServices |
|-------------|------|--------|---------|-----------|-----------------|
| 0x01 | DefaultSession | 50 | 5000 | 5000 | 10,11,19,22,3E |
| 0x02 | ProgrammingSession | 50 | 5000 | 5000 | 10,11,27,31,34,35,36,37 |
| 0x03 | ExtendedSession | 50 | 5000 | 5000 | 10,11,19,22,2E,27,31,3E |

**DIDConfig 示例：**

| DID(Hex) | Name | DataLength | ReadOnly | SecurityLevel | SessionMask | DataType |
|----------|------|------------|----------|---------------|-------------|----------|
| 0xF190 | VIN | 17 | Yes | 0x01 | 0x07 | ASCII |
| 0xF197 | ECU_Name | 16 | Yes | 0x01 | 0x07 | ASCII |
| 0xF100 | SystemState | 4 | No | 0x03 | 0x06 | HEX |

**生成文件：**
- `UdsConfig_Generated.h` - UDS 配置头文件
- `UdsConfig_Generated.c` - UDS 配置实现
- `INTEGRATION.md` - 集成指南

---

## 🔨 2. build_tools - 构建烧录工具

### 2.1 build.sh - 项目编译

编译 STM32 项目。

**用法：**
```bash
cd build_tools
./build.sh [debug|release]
```

**参数：**
- `debug` - 编译调试版本（默认）
- `release` - 编译发布版本

**示例：**
```bash
./build.sh              # 编译调试版本
./build.sh release      # 编译发布版本
```

---

### 2.2 flash.sh - 烧录固件

通过 J-Link 烧录固件到 STM32。

**用法：**
```bash
cd build_tools
./flash.sh [hex|bin|erase]
```

**参数：**
- `hex` - 烧录 HEX 文件（默认）
- `bin` - 烧录 BIN 文件
- `erase` - 擦除芯片

**示例：**
```bash
./flash.sh          # 烧录 HEX 文件
./flash.sh bin      # 烧录 BIN 文件
./flash.sh erase    # 擦除整个芯片
```

---

### 2.3 flash_check.sh - 烧录检查

检查烧录是否成功，验证 Flash 内容。

**用法：**
```bash
cd build_tools
./flash_check.sh
```

**输出：**
- 显示烧录的固件信息
- 验证 Flash 校验和
- 报告烧录状态

---

### 2.4 verify_flash.sh - 验证烧录

详细验证 Flash 内容，与原始文件对比。

**用法：**
```bash
cd build_tools
./verify_flash.sh <原始文件> [地址]
```

**参数：**
- `<原始文件>` - 原始二进制文件路径
- `[地址]` - Flash 起始地址（默认 0x08000000）

**示例：**
```bash
./verify_flash.sh ../build/STM32F407_CAN.bin
./verify_flash.sh ../build/STM32F407_CAN.bin 0x08000000
```

---

## 🐛 3. debug_tools - 调试工具

### 3.1 debug.sh - 启动调试

启动 OpenOCD GDB 调试服务器。

**用法：**
```bash
cd debug_tools
./debug.sh [port]
```

**参数：**
- `[port]` - GDB 端口号（默认 3333）

**示例：**
```bash
./debug.sh          # 在端口 3333 启动调试服务器
./debug.sh 3334     # 在端口 3334 启动调试服务器
```

**配合 GDB 使用：**
```bash
# 终端1：启动调试服务器
./debug.sh

# 终端2：连接 GDB
arm-none-eabi-gdb ../build/STM32F407_CAN.elf
gdb> target remote localhost:3333
gdb> load
gdb> continue
```

---

### 3.2 setup_can.sh - CAN 接口配置

配置 Linux CAN 接口。

**用法：**
```bash
cd debug_tools
sudo ./setup_can.sh <interface> <bitrate> [mode]
```

**参数：**
- `<interface>` - CAN 接口名（如 can0, can1）
- `<bitrate>` - 波特率（如 500000, 1000000）
- `[mode]` - 模式（fd 或 classic，默认 classic）

**示例：**
```bash
# 配置 CAN0 为经典 CAN，500Kbps
sudo ./setup_can.sh can0 500000

# 配置 CAN1 为 CAN FD，1Mbps
sudo ./setup_can.sh can1 1000000 fd

# 关闭 CAN 接口
sudo ./setup_can.sh can0 down
```

**检查 CAN 状态：**
```bash
ip -details link show can0
```

---

## 🔍 4. diagnostic_tools - 诊断测试工具

### 4.1 can_monitor.py - CAN 总线监控

实时监控 CAN 总线上的所有帧。

**用法：**
```bash
cd diagnostic_tools
python3 can_monitor.py <interface>
```

**参数：**
- `<interface>` - CAN 接口名（如 can0）

**示例：**
```bash
# 监控 can0 接口
python3 can_monitor.py can0

# 监控 can1 接口
python3 can_monitor.py can1
```

**输出示例：**
```
(1234567890.123456)  can0  210   [8]  01 00 00 00 00 00 00 00
(1234567890.234567)  can0  211   [8]  04 00 00 00 00 00 00 00
(1234567890.345678)  can0  73D   [3]  02 7E 00
```

---

### 4.2 diag_test.py - UDS 诊断测试

自动化 UDS 诊断服务测试工具。

**用法：**
```bash
cd diagnostic_tools
python3 diag_test.py <interface>
```

**参数：**
- `<interface>` - CAN 接口名（如 can0）

**示例：**
```bash
python3 diag_test.py can0
```

**测试内容：**
1. **0x10** - DiagnosticSessionControl（会话控制）
2. **0x11** - ECUReset（ECU 复位）
3. **0x22** - ReadDataByIdentifier（读取 DID）
   - VIN (0xF190)
   - ECU Name (0xF197)
   - System State (0xF100)
4. **0x3E** - TesterPresent（诊断仪在线）
5. **0x27** - SecurityAccess（安全访问）

**输出示例：**
```
=== DBC Matrix Diagnostic Test ===
RX: 0x7DF (Functional), 0x735 (Physical)
TX: 0x73D (Response)

[TEST 1] 0x10 - DiagnosticSessionControl
Sending: 10 01 (Default Session)
Response: 50 01 00 32 13 88  ✓

[TEST 2] 0x22 - ReadDataByIdentifier
Sending: 22 F1 90 (Read VIN)
Response: 62 F1 90 31 32 33...  ✓
```

---

## 📋 5. config - 配置文件

### XZ_CAN_V2.xlsx

DBC 矩阵配置表，定义 CAN 通信协议。

**包含工作表：**
- **Messages** - CAN 消息定义
- **Signals** - 信号定义
- **Diag** - 诊断相关定义

**使用方法：**
1. 使用 Excel 编辑 `config/XZ_CAN_V2.xlsx`
2. 运行 `code_generators/generate_dbc.sh` 生成代码
3. 重新编译项目

---

## 🚀 快速开始示例

### 完整开发流程示例

```bash
# 1. 进入工具目录
cd <your-project-path>/STM32_AUTOSAR/tools

# 2. 配置 CAN 接口
sudo ./debug_tools/setup_can.sh can0 500000

# 3. 生成 DBC 代码
./code_generators/generate_dbc.sh

# 4. 生成 UDS 配置（首次需要创建示例）
./code_generators/uds_generator/generate_uds.sh --example
# 编辑 UDS_Config_Example.xlsx 后：
./code_generators/uds_generator/generate_uds.sh \
    ./code_generators/uds_generator/UDS_Config_Example.xlsx

# 5. 编译项目
./build_tools/build.sh

# 6. 烧录固件
./build_tools/flash.sh

# 7. 监控 CAN 数据
./diagnostic_tools/can_monitor.py can0

# 8. 运行诊断测试
./diagnostic_tools/diag_test.py can0
```

---

## 🛠️ 环境要求

### 必需软件

| 软件 | 版本 | 安装命令 |
|------|------|----------|
| Python | 3.6+ | `sudo apt-get install python3` |
| openpyxl | - | `pip3 install openpyxl` |
| can-utils | - | `sudo apt-get install can-utils` |
| OpenOCD | 0.12.0+ | `sudo apt-get install openocd` |
| arm-none-eabi-gcc | 13.2+ | `sudo apt-get install gcc-arm-none-eabi` |

### 检查安装

```bash
# 检查 Python
python3 --version

# 检查 openpyxl
python3 -c "import openpyxl; print('openpyxl OK')"

# 检查 can-utils
candump --version

# 检查 OpenOCD
openocd --version

# 检查交叉编译器
arm-none-eabi-gcc --version
```

---

## 📝 添加新工具

如需添加新工具，请遵循以下规范：

1. **选择合适分类**
   - 代码生成 → `code_generators/`
   - 构建烧录 → `build_tools/`
   - 调试用具 → `debug_tools/`
   - 诊断测试 → `diagnostic_tools/`
   - 配置文件 → `config/`

2. **独立文件夹**
   如果工具包含多个文件，创建独立文件夹：
   ```
   code_generators/
   └── my_generator/
       ├── my_generator.py
       ├── generate.sh
       └── README.md
   ```

3. **添加说明文档**
   在工具目录添加 `README.md`，包含：
   - 功能说明
   - 使用方法
   - 参数说明
   - 示例

4. **更新本文件**
   在 `README.md` 对应分类中添加新工具说明

---

## 📞 支持与反馈

如有问题或建议，请参考：
- DBC 生成器：查看 `code_generators/README_DBC_GENERATOR.md`
- UDS 生成器：查看 `code_generators/uds_generator/README.md`

---

**版本：** v1.0  
**更新日期：** 2026-03-15
