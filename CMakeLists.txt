# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(tinyscopepico C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(SOURCES
    ssd1306.c
    capture.cpp
    scope.cpp
    tinyscopepico.cpp
)

# Add executable. Default name is the project name, version 0.1

add_executable(tinyscopepico  ${SOURCES})
pico_enable_stdio_usb(tinyscopepico 1)
pico_set_program_name(tinyscopepico "tinyscopepico")
pico_set_program_version(tinyscopepico "0.1")

# Modify the below lines to enable/disable output over UART/USB
add_compile_definitions(PICO_STDIO_USB=1)
#add_compile_definitions(PICO_STDIO_UART=0) 

# Add the standard library to the build
target_link_libraries(tinyscopepico
        pico_stdlib
        hardware_adc)

# Add the standard include files to the build
target_include_directories(tinyscopepico PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(tinyscopepico 
        hardware_i2c
        hardware_timer
        hardware_watchdog
        pico_stdlib
        )

pico_add_extra_outputs(tinyscopepico)

