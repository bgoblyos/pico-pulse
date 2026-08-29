# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(pico-pulse VERSION 0.1 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Generate version header
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/version.h.in version.h)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# turn on all compiler warnings 
add_compile_options(-Wall)

add_executable(pico-pulse)

pico_generate_pio_header(pico-pulse ${CMAKE_CURRENT_LIST_DIR}/src/pico-pulse.pio)

# Default to rev_a if not specified
set(BOARD_REV "bare" CACHE STRING "Target board hardware revision")

# Validate directory exists
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD_REV}")
    message(FATAL_ERROR "Unknown board revision: ${BOARD_REV}")
endif()

message(STATUS "Building for Hardware Revision: ${BOARD_REV}")

# Add the selected board directory to target includes
target_include_directories(pico-pulse PRIVATE 
    "boards/${BOARD_REV}"
)

target_sources(pico-pulse PRIVATE src/main.c src/hardware.c src/command.c src/pulse.c src/status.c src/rheostat.c src/laser.c)

target_link_libraries(pico-pulse PRIVATE pico_stdlib pico_unique_id hardware_pio hardware_dma hardware_i2c)

pico_enable_stdio_usb(pico-pulse 1)

# Build UF2 files as well
pico_add_extra_outputs(pico-pulse)

target_compile_definitions(pico-pulse PRIVATE
        PLL_SYS_REFDIV=1
        PLL_SYS_VCO_FREQ_HZ=1200000000
        PLL_SYS_POSTDIV1=6
        PLL_SYS_POSTDIV2=1
        SYS_CLK_HZ=200000000
)

