# Bootloader CMakeLists.txt
# STM32F407 Bootloader - 64KB @ 0x08000000

cmake_minimum_required(VERSION 3.20)

# Set cross compiler before project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(Bootloader C ASM)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")

# MCU Settings
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -std=c99 -Wall -fdata-sections -ffunction-sections -g")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs")

# Include directories
include_directories(
    include
    ../AUTOSAR/Std
    ../include
)

# Bootloader sources
set(BOOTLOADER_SOURCES
    src/main_bootloader.c
    src/boot.c
    src/flash_drv.c
    src/crc32.c
    src/uds_bootloader.c
    ../src/board/system_stm32f4xx.c
)

# Startup file (shared with main project)
set(STARTUP_FILE ../src/board/startup_stm32f407xx.s)

# Add executable
add_executable(${PROJECT_NAME}.elf 
    ${BOOTLOADER_SOURCES}
    ${STARTUP_FILE}
)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/ld/STM32F407_Bootloader.ld)
target_link_options(${PROJECT_NAME}.elf PRIVATE -T${LINKER_SCRIPT})

# Post-build commands
set(ELF_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf)
set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex ${ELF_FILE} ${HEX_FILE}
    COMMAND arm-none-eabi-objcopy -O binary ${ELF_FILE} ${BIN_FILE}
    COMMAND arm-none-eabi-size ${ELF_FILE}
    COMMENT "Generating HEX and BIN files"
)

# Flash targets
add_custom_target(flash
    COMMAND openocd -f interface/jlink.cfg -c "transport select swd" 
            -f target/stm32f4x.cfg 
            -c "program ${BIN_FILE} 0x08000000 verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing Bootloader to 0x08000000"
)

add_custom_target(erase
    COMMAND openocd -f interface/jlink.cfg -c "transport select swd"
            -f target/stm32f4x.cfg
            -c "init; reset halt"
            -c "flash erase_sector 0 0 3"
            -c "shutdown"
    COMMENT "Erasing Bootloader sectors"
)
