# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "C:/Users/tim.KOHLSTEDT0/Documents/Pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

set(TARGET_NAME PicoPlayer)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
 
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(PicoPlayer C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(FatFs_SPI build)

# Add executable. Default name is the project name, version 0.1

add_executable(${TARGET_NAME}
  ${TARGET_NAME}.c
  hw_config.c
  fseq.c
  config.c
  outputManager.c
)
pico_set_program_name(PicoPlayer "PicoPlayer")
pico_set_program_version(PicoPlayer "0.1")
pico_enable_stdio_uart(PicoPlayer 1)
pico_enable_stdio_usb(PicoPlayer 1)

# generate the header file into the source tree as it is included in the RP2040 datasheet
pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

# Add the standard library to the build
target_link_libraries(PicoPlayer
        pico_stdlib
        FatFs_SPI)
# Add the standard include files to the build
target_include_directories(PicoPlayer PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)
# Add any user requested libraries
target_link_libraries(PicoPlayer
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_adc
        )
pico_add_extra_outputs(PicoPlayer)

add_executable(FatFS_SPI_example 
    FatFS_SPI_example.cpp 
    hw_config.c
    ./no-OS-FatFS/example/data_log_demo.c
    ./no-OS-FatFS/example/tests/simple.c
    ./no-OS-FatFS/example/tests/app4-IO_module_function_checker.c
    ./no-OS-FatFS/example/tests/big_file_test.c
    ./no-OS-FatFS/example/tests/CreateAndVerifyExampleFiles.c
    ./no-OS-FatFS/example/tests/ff_stdio_tests_with_cwd.c
)

pico_set_program_name(FatFS_SPI_example "FatFS_SPI_example")
pico_set_program_version(FatFS_SPI_example "0.1")

pico_enable_stdio_uart(FatFS_SPI_example 1)
pico_enable_stdio_usb(FatFS_SPI_example 1)

target_link_libraries(FatFS_SPI_example
        pico_stdlib
        FatFs_SPI)

target_include_directories(FatFS_SPI_example PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

target_link_libraries(FatFS_SPI_example
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_adc
        )

pico_add_extra_outputs(FatFS_SPI_example)
