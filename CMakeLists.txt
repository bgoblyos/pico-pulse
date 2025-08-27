# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(modulator C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# turn on all compiler warnings 
add_compile_options(-Wall)

add_executable(pico-pulse)

pico_generate_pio_header(pico-pulse ${CMAKE_CURRENT_LIST_DIR}/src/pico-pulse.pio)

target_sources(pico-pulse PRIVATE src/pico-pulse.c src/hardware.c)

target_link_libraries(pico-pulse PRIVATE pico_stdlib pico_unique_id hardware_pio hardware_dma)

pico_enable_stdio_usb(pico-pulse 1)

# Build UF2 files as well
# pico_add_extra_outputs(pico-pulse)
