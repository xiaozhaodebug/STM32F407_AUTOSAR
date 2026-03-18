# STM32F407 CAN Communication Project
# CMake Build Configuration

cmake_minimum_required(VERSION 3.16)

# Prevent in-source build
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source build is not allowed. Please create a build directory.")
endif()

# Set cross compiler before project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Project definition
project(STM32F407_CAN
    VERSION 1.0.0
    LANGUAGES C ASM
    DESCRIPTION "STM32F407 CAN Communication Project"
)

#-------------------------------------------------------------------------------
# Cross Compiler Configuration
#-------------------------------------------------------------------------------

# Set ARM specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcpu=cortex-m4 -mthumb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs -specs=nosys.specs -lm")

#-------------------------------------------------------------------------------
# Build Type Configuration
#-------------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_BUILD_TYPE_OPTIONS "Debug;Release;MinSizeRel")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_BUILD_TYPE_OPTIONS})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#-------------------------------------------------------------------------------
# Build Flags Configuration
#-------------------------------------------------------------------------------

# Debug build flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG")

# Release build flags
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# MinSizeRel flags (for smallest code size)
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Additional warning flags (C89 compliance)
add_compile_options(
    -Wextra
    -Wshadow
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wredundant-decls
    -Wnested-externs
    -Winline
    -Wno-long-long
)

#-------------------------------------------------------------------------------
# Project Configuration
#-------------------------------------------------------------------------------

# Target name
set(TARGET_NAME "${PROJECT_NAME}")

# App Version (for OTA upgrade verification)
set(APP_VERSION_MAJOR "1" CACHE STRING "App Major Version")
set(APP_VERSION_MINOR "0" CACHE STRING "App Minor Version")
add_compile_definitions(
    APP_VERSION_MAJOR=${APP_VERSION_MAJOR}
    APP_VERSION_MINOR=${APP_VERSION_MINOR}
)
message(STATUS "App Version: ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}")

# AUTOSAR Integration Option
option(ENABLE_AUTOSAR "Enable AUTOSAR stack integration" ON)

# Source files
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board/startup_stm32f407xx.s
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/CanDriver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/DebugLog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/DbcHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/BootloaderJump.c
)

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Std
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/CanIf
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/PduR
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Com
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Dcm
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/EcuM
)

# AUTOSAR Stack Integration
if(ENABLE_AUTOSAR)
    message(STATUS "AUTOSAR stack integration: ENABLED")
    
    # AUTOSAR source files
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Std/Std_Types.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/CanIf/CanIf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/PduR/PduR.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Com/Com.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Dcm/Dcm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/EcuM/EcuM.c
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/UdsConfig/UdsConfig_Generated.c
    )
    
    # AUTOSAR include directories
    list(APPEND INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Std
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/CanIf
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/PduR
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Com
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/Dcm
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/EcuM
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR/UdsConfig
        ${CMAKE_CURRENT_SOURCE_DIR}/AUTOSAR
    )
    
    add_definitions(-DENABLE_AUTOSAR=1)
else()
    message(STATUS "AUTOSAR stack integration: DISABLED")
endif()

#-------------------------------------------------------------------------------
# Executable Target
#-------------------------------------------------------------------------------

add_executable(${TARGET_NAME}.elf ${SOURCES})

target_include_directories(${TARGET_NAME}.elf PRIVATE ${INCLUDE_DIRS})

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F407ZGTx_FLASH.ld)
if(EXISTS ${LINKER_SCRIPT})
    target_link_options(${TARGET_NAME}.elf PRIVATE
        -T${LINKER_SCRIPT}
        -Wl,-Map=${TARGET_NAME}.map
    )
    set_target_properties(${TARGET_NAME}.elf PROPERTIES
        LINK_DEPENDS ${LINKER_SCRIPT}
    )
endif()

#-------------------------------------------------------------------------------
# Post-Build Commands (Generate hex, bin, and show size)
#-------------------------------------------------------------------------------

add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}.elf> ${TARGET_NAME}.hex
    COMMENT "Generating HEX file: ${TARGET_NAME}.hex"
)

add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}.elf> ${TARGET_NAME}.bin
    COMMENT "Generating BIN file: ${TARGET_NAME}.bin"
)

add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${TARGET_NAME}.elf>
    COMMENT "Memory usage:"
)

#-------------------------------------------------------------------------------
# Flash Targets
#-------------------------------------------------------------------------------

# Flash via OpenOCD (J-Link SWD)
add_custom_target(flash
    COMMENT "Flashing firmware via OpenOCD (J-Link SWD)..."
    COMMAND openocd
        -f interface/jlink.cfg
        -c "transport select swd"
        -f target/stm32f4x.cfg
        -c "program $<TARGET_FILE:${TARGET_NAME}.elf> verify reset exit"
    DEPENDS ${TARGET_NAME}.elf
)

# Flash App via OpenOCD with specific address (for use with Bootloader)
# Note: App starts at 0x08010000 (after 64KB Bootloader)
add_custom_target(flash-app
    COMMENT "Flashing Application to 0x08010000 via OpenOCD (J-Link SWD)..."
    COMMAND openocd
        -f interface/jlink.cfg
        -c "transport select swd"
        -f target/stm32f4x.cfg
        -c "init"
        -c "reset halt"
        -c "flash write_image erase ${TARGET_NAME}.bin 0x08010000"
        -c "verify_image ${TARGET_NAME}.bin 0x08010000"
        -c "reset run"
        -c "shutdown"
    DEPENDS ${TARGET_NAME}.elf
)

# Erase entire chip
add_custom_target(erase
    COMMENT "Erasing chip..."
    COMMAND openocd
        -f interface/jlink.cfg
        -c "transport select swd"
        -f target/stm32f4x.cfg
        -c "init"
        -c "reset halt"
        -c "flash erase_sector 0 0 last"
        -c "shutdown"
)

# Reset target
add_custom_target(reset
    COMMENT "Resetting target..."
    COMMAND openocd
        -f interface/jlink.cfg
        -c "transport select swd"
        -f target/stm32f4x.cfg
        -c "init"
        -c "reset run"
        -c "shutdown"
)

# Debug server (GDB via OpenOCD)
add_custom_target(debug-server
    COMMENT "Starting OpenOCD GDB server (J-Link SWD)..."
    COMMAND openocd
        -f interface/jlink.cfg
        -c "transport select swd"
        -f target/stm32f4x.cfg
)

# GDB client
add_custom_target(gdb
    COMMENT "Starting GDB client..."
    COMMAND ${CMAKE_GDB} -ex "target remote localhost:3333" -ex "monitor reset halt"
        $<TARGET_FILE:${TARGET_NAME}.elf>
)

#-------------------------------------------------------------------------------
# Utility Targets
#-------------------------------------------------------------------------------

# Disassembly
add_custom_target(disasm
    COMMENT "Generating disassembly..."
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${TARGET_NAME}.elf> > ${TARGET_NAME}.lst
    DEPENDS ${TARGET_NAME}.elf
)

# Clean all generated files
add_custom_target(clean-all
    COMMENT "Cleaning all generated files..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
)

#-------------------------------------------------------------------------------
# Print Configuration Summary
#-------------------------------------------------------------------------------

message(STATUS "")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Target MCU: STM32F407ZGT6")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Linker Script: ${LINKER_SCRIPT}")
message(STATUS "Available targets:")
message(STATUS "  - ${TARGET_NAME}.elf : Build firmware")
message(STATUS "  - flash              : Flash firmware via OpenOCD")
message(STATUS "  - flash-bin          : Flash BIN file via OpenOCD")
message(STATUS "  - erase              : Erase entire chip")
message(STATUS "  - reset              : Reset target")
message(STATUS "  - debug-server       : Start OpenOCD GDB server")
message(STATUS "  - gdb                : Connect GDB to target")
message(STATUS "  - disasm             : Generate disassembly listing")
message(STATUS "")
