cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(main)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

include("PicoLED/PicoLed.cmake")

add_executable(main
    main.cpp
)

pico_enable_stdio_usb(main 1)

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_adc PicoLed)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(main pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(main)
