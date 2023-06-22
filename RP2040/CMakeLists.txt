# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/lukas/RaspberryPiPico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")
set(CMAKE_BUILD_TYPE "Release")
#set(PICO_DEOPTIMIZED_DEBUG 1)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(StepperClock C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(StepperClock
        StepperClock.c
        StepperClock_Config.c
        StepperClock_PWM_STEPPER.c
        StepperClock_PWM_PENDULUM.c
        StepperClock_Trinamic.c
        StepperClock_RTC.c

        TMC2209Implementation.c

        TMC-API/tmc/helpers/CRC.c
        TMC-API/tmc/helpers/Functions.c
        TMC-API/tmc/ic/TMC2209/TMC2209.c
)

pico_set_program_name(StepperClock "StepperClock")
pico_set_program_version(StepperClock "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(StepperClock no_flash)

pico_enable_stdio_uart(StepperClock 0)
pico_enable_stdio_usb(StepperClock 1)

# Add the standard library to the build
target_link_libraries(StepperClock pico_stdlib)

# Add the standard include files to the build
target_include_directories(StepperClock PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
  ${CMAKE_CURRENT_LIST_DIR}/TMC-API
)

# Add any user requested libraries
target_link_libraries(StepperClock
        hardware_pwm
        hardware_timer
        hardware_rtc)

pico_add_extra_outputs(StepperClock)

