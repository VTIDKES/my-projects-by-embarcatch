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
cmake_minimum_required(VERSION 3.13)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Determine user home directory based on the operating system
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

# Set SDK versions
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)

# Include Pico SDK configuration for VS Code
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Specify the board type
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Include Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

# Define the project
project(TAREFA7 C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable target
add_executable(TAREFA7 TAREFA7.c libs/ssd1306.c)

# Set program name and version
pico_set_program_name(TAREFA7 "TAREFA7")
pico_set_program_version(TAREFA7 "0.1")

# Generate PIO header
pico_generate_pio_header(TAREFA7 ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Configure UART/USB output options
pico_enable_stdio_uart(TAREFA7 0)  # Disable UART output
pico_enable_stdio_usb(TAREFA7 1)    # Enable USB output

# Link standard libraries and hardware components
target_link_libraries(TAREFA7
        pico_stdlib
        hardware_adc
        hardware_i2c
        hardware_pio
        pico_lwip_http
        pico_cyw43_arch_lwip_threadsafe_background
    )

# Include directories for the target
target_include_directories(TAREFA7 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        libs
)

# Generate extra outputs (e.g., hex, bin files)
pico_add_extra_outputs(TAREFA7)
