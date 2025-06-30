cmake_minimum_required(VERSION 3.22)

include("cmake/gcc-arm-none-eabi.cmake")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(
    MCUCMake
    LANGUAGES C CXX ASM
)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

add_subdirectory(cmake/MSPM0G3507)

#---------------------------------------------------------------------
# Configuration related to SysConfig
#---------------------------------------------------------------------
# Find the SysConfig command line interface (CLI) command in the system path
set(SYSCONFIG_CLI ${SYSCONFIG_ROOT}/sysconfig_cli.bat)

# Directory to the generated SysConfig files
set(SYSCONFIG_GENDIR ${CMAKE_BINARY_DIR}/sysconfig_generated)

# SysConfig Input Script
set(SYSCONFIG_INPUT_FILE_NAME ${PROJECT_NAME}.syscfg)
set(SYSCONFIG_INPUT ${CMAKE_SOURCE_DIR}/${SYSCONFIG_INPUT_FILE_NAME})

# SysConfig stamp file.  Described in the block comment at the end.
set(SYSCONFIG_STAMP_FILE ${SYSCONFIG_GENDIR}/${SYSCONFIG_INPUT_FILE_NAME}.stamp)

# SysConfig invocation command line
set(SYSCONFIG_CLI_CMD
    ${SYSCONFIG_CLI}
        --script ${SYSCONFIG_INPUT}
        -o ${SYSCONFIG_GENDIR}
        --compiler gcc
        -s ${MSPM0_SDK_ROOT}/.metadata/product.json
)

# Ask SysConfig what files it generates.  Then categorize them.  This occurs
# when CMake configures the project.
execute_process(
    COMMAND ${SYSCONFIG_CLI_CMD} --listGeneratedFiles
    OUTPUT_VARIABLE SYSCONFIG_GEN_FILES_STR
)
string(REPLACE "\n" ";" SYSCONFIG_GEN_FILES ${SYSCONFIG_GEN_FILES_STR})

set(SYSCONFIG_GEN_CFILES        ${SYSCONFIG_GEN_FILES})
set(SYSCONFIG_GEN_HFILES        ${SYSCONFIG_GEN_FILES})
set(SYSCONFIG_GEN_LNKCMD        ${SYSCONFIG_GEN_FILES})
set(SYSCONFIG_GEN_COMPILER_OPTS ${SYSCONFIG_GEN_FILES})

list(FILTER SYSCONFIG_GEN_CFILES        INCLUDE REGEX ".*\\.c$")
list(FILTER SYSCONFIG_GEN_HFILES        INCLUDE REGEX ".*\\.h$")
list(FILTER SYSCONFIG_GEN_LNKCMD        INCLUDE REGEX ".*\\.cmd\\.")
list(FILTER SYSCONFIG_GEN_COMPILER_OPTS INCLUDE REGEX ".*\\.opt$")

# Cause CMake to run when the SysConfig input is modified, because that may
# change the list of generated files
set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${SYSCONFIG_INPUT}
)

# Run SysConfig over the input script.  Described further in a block comment
# at the end.
add_custom_command(
    DEPENDS    ${SYSCONFIG_INPUT}
    OUTPUT     ${SYSCONFIG_STAMP_FILE}
    BYPRODUCTS ${SYSCONFIG_GEN_FILES}
    COMMAND    ${SYSCONFIG_CLI_CMD}
    COMMAND    ${CMAKE_COMMAND} -E touch ${SYSCONFIG_STAMP_FILE}
    COMMENT    "Running SysConfig on ${SYSCONFIG_INPUT}"
    VERBATIM
)
add_custom_target(SysConfig_build ALL DEPENDS ${SYSCONFIG_STAMP_FILE})

#---------------------------------------------------------------------
# Configuration related to compiling
#---------------------------------------------------------------------
# Compiler options which specify the variant of the Arm processor in the
# system.  These are often copied from an SDK example for that system.  They
# rarely change.
set(PROCESSOR_OPTIONS
    -march=armv6-m
    -mcpu=cortex-m0plus
    -mfloat-abi=soft
    -mthumb
    -g
    -Wall
    -lgcc
    -lc 
    -lm
)

# These compiler options typically change as the project evolves.  Because
# CMake replaces an undefined variable with an empty string, one way to
# disable an option is to comment out that line.
set(OPTIMIZATION_OPTION -Oz)
set(LTO_OPTION          -flto)
set(DEBUG_OPTION        -gdwarf)

# Compiler options
add_compile_options(
    ${PROCESSOR_OPTIONS}
    ${OPTIMIZATION_OPTION}
    ${LTO_OPTION}
    ${DEBUG_OPTION}
    @${SYSCONFIG_GEN_COMPILER_OPTS}
)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    Application/*.c
    Application/**/*.c
    Hardware/*.c
    Hardware/**/*.c
    Startup/startup_mspm0g350x_gcc.c
)

# C files
set(SOURCES
    ${PROJECT_SOURCES}
    ${SYSCONFIG_GEN_CFILES}
)

# Cause C files to depend on the SysConfig generated compiler options file
set_source_files_properties(
    ${SOURCES}
    PROPERTIES OBJECT_DEPENDS
    ${SYSCONFIG_GEN_COMPILER_OPTS}
)

# Directories searched for header files
include_directories(
    Application
    Hardware
    ${SYSCONFIG_GENDIR}
)

#---------------------------------------------------------------------
# Configuration related to linking
#---------------------------------------------------------------------
# Linker Options
add_link_options(
    ${LTO_OPTION}            # Required when compiling AND linking
    -T${SYSCONFIG_GENDIR}/device_linker.lds
    -static
    -nostartfiles
    -march=armv6-m
    -mthumb
)

#---------------------------------------------------------------------
# Final configuration that pulls everything together
#---------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
    ${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE driverlib)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
)