cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(RPCommon C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(rp_common
        adc.cpp
        adc.hpp
        deep_sleep.cpp
        deep_sleep.hpp
        flash_storage.cpp
        flash_storage.hpp
        i2c.cpp
        i2c.hpp
        tcp_client.cpp
        tcp_client.hpp
        wireless.cpp
        wireless.hpp)

target_link_libraries(rp_common
        pico_stdlib
        hardware_pio
        hardware_dma)

target_include_directories(rp_common PRIVATE .)
